#ifndef YANTA_TOOLS_H__
#define YANTA_TOOLS_H__

#include <list>

#include <pipeline/Data.h>

#include "Tool.h"

class Tools : public pipeline::Data {

public:

	typedef std::list<boost::shared_ptr<Tool> > container;
	typedef typename container::iterator        iterator;
	typedef typename container::const_iterator  const_iterator;

	/**
	 * Add a tool to the document.
	 */
	void add(boost::shared_ptr<Tool> tool) { _tools.push_back(tool); }

	/**
	 * Iterator access to the tools.
	 */
	iterator begin() { return _tools.begin(); }
	const_iterator begin() const { return _tools.begin(); }
	iterator end() { return _tools.end(); }
	const_iterator end() const { return _tools.end(); }

	/**
	 * Remove a tool from the document.
	 */
	void remove(boost::shared_ptr<Tool> tool) {

		iterator i = std::find(_tools.begin(), _tools.end(), tool);
		
		if (i != _tools.end())
			_tools.erase(i);
	}

	/**
	 * Get the number of tools.
	 */
	unsigned int size() const { return _tools.size(); }

private:

	// the current tools
	container _tools;
};

#endif // YANTA_TOOLS_H__

