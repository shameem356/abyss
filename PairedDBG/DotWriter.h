#ifndef PAIREDDBG_DOTWRITER_H
#define PAIREDDBG_DOTWRITER_H 1

/** Written by Shaun Jackman <sjackman@bcgsc.ca>. */

#include "SequenceCollection.h"
#include "Common/UnorderedMap.h"
#include "Graph/ContigGraphAlgorithms.h"
#include <cassert>
#include <iostream>
#include <sstream>

class DotWriter
{
private:
	typedef SequenceCollectionHash Graph;
	typedef graph_traits<Graph>::vertex_descriptor vertex_descriptor;
	typedef graph_traits<Graph>::vertex_iterator vertex_iterator;
	typedef graph_traits<Graph>::adjacency_iterator adjacency_iterator;
	typedef std::string VertexName;

	DotWriter() : m_id(0) { }

/** Complement the specified name. */
static std::string
complementName(std::string s)
{
	char c = s.back();
	assert(c == '+' || c == '-');
	s.back() = c == '+' ? '-' : '+';
	return s;
}

/** Return the name of the specified vertex. */
const VertexName&
getName(const vertex_descriptor& u) const
{
	Names::const_iterator it = m_names.find(u);
	if (it == m_names.end())
		std::cerr << "error: cannot find vertex " << u << '\n';
	assert(it != m_names.end());
	return it->second;
}

/** Set the name of the specified vertex. */
void setName(const vertex_descriptor& u, const VertexName& uname)
{
	std::pair<Names::const_iterator, bool> inserted
		= m_names.insert(Names::value_type(u, uname));
	if (!inserted.second)
		std::cerr << "error: duplicate vertex " << u << '\n';
	assert(inserted.second);
	(void)inserted;
}

/** Write out the specified contig. */
void writeContig(std::ostream& out, const Graph& g, const vertex_descriptor& u)
{
	unsigned n = 1;
	vertex_descriptor v = u;
	while (contiguous_out(g, v)) {
		n++;
		v = *adjacent_vertices(v, g).first;
	}

	// Output the canonical orientation of the contig.
	vertex_descriptor vrc = get(vertex_complement, g, v);
	if (vrc < u)
		return;

	std::ostringstream ss;
	ss << m_id << '+';
	VertexName uname = ss.str();
	VertexName vname(uname);
	vname.back() = '-';
	++m_id;

	setName(u, uname);
	if (u == vrc) {
		// Palindrome
		assert(n == 1);
	} else
		setName(vrc, vname);

	unsigned l = n + vertex_descriptor::length() - 1;
	out << '"' << uname << "\" [l=" << l << "]\n";
	out << '"' << vname << "\" [l=" << l << "]\n";
}

/** Write out the contigs that split at the specified sequence. */
void
writeEdges(std::ostream& out, const Graph& g,
		const vertex_descriptor& u, const VertexName& uname) const
{
	if (out_degree(u, g) == 0)
		return;
	out << '"' << uname << "\" -> {";
	std::pair<adjacency_iterator, adjacency_iterator>
		adj = adjacent_vertices(u, g);
	for (adjacency_iterator vit = adj.first; vit != adj.second; ++vit) {
		vertex_descriptor v = *vit;
		const VertexName& vname = getName(v);
		out << " \"" << vname << '"';
		if (v.isPalindrome())
			out << " \"" << complementName(vname) << '"';
	}
	out << " }\n";
}

/** Output the edges of the specified vertex. */
void
writeEdges(std::ostream& out, const Graph& g, const vertex_descriptor& u) const
{
	std::string uname = complementName(getName(get(vertex_complement, g, u)));
	writeEdges(out, g, u, uname);
	if (u.isPalindrome()) {
		uname = complementName(uname);
		writeEdges(out, g, u, uname);
	}
}

/** Write out a dot graph for the specified collection. */
void writeGraph(std::ostream& out, const Graph& g)
{
	out << "digraph g {\n";
	std::pair<vertex_iterator, vertex_iterator> uits = vertices(g);

	// Output the vertices.
	for (vertex_iterator uit = uits.first; uit != uits.second; ++uit) {
		vertex_descriptor u = *uit;
		if (get(vertex_removed, g, u))
			continue;
		if (!contiguous_in(g, u))
			writeContig(out, g, u);
		// Skip the second occurence of the palindrome.
		if (u.isPalindrome()) {
			assert(uit != uits.second);
			++uit;
		}
	}

	// Output the edges.
	for (vertex_iterator uit = uits.first; uit != uits.second; ++uit) {
		vertex_descriptor u = *uit;
		if (get(vertex_removed, g, u))
			continue;
		if (!contiguous_out(g, u))
			writeEdges(out, g, u);
		// Skip the second occurence of the palindrome.
		if (u.isPalindrome()) {
			assert(uit != uits.second);
			++uit;
		}
	}

	out << "}" << std::endl;
}

public:

/** Write out a dot graph for the specified collection. */
static
void write(std::ostream& out, const Graph& g)
{
	DotWriter dotWriter;
	dotWriter.writeGraph(out, g);
}

private:
	typedef unordered_map<vertex_descriptor, VertexName> Names;

	/** A map of terminal k-mers to contig names. */
	Names m_names;

	/** The current contig name. */
	unsigned m_id;
};

#endif
