#ifndef YANTA_BACKEND_H__
#define YANTA_BACKEND_H__

#include <gui/PenSignals.h>
#include <pipeline/all.h>

#include <document/DocumentSignals.h>
#include <document/Selection.h>
#include <gui/BackendPainter.h>
#include <gui/OsdRequest.h>
#include <gui/OsdSignals.h>
#include <gui/Overlay.h>
#include <gui/OverlaySignals.h>
#include <tools/PenMode.h>
#include <tools/Lasso.h>

class Backend : public pipeline::SimpleProcessNode<> {

public:

	Backend();

	/**
	 * Finish pending operations like anchoring floating selections. This will 
	 * be called before the application saves and quits.
	 */
	void cleanup();

private:

	enum Mode {

		Draw,
		Erase,
		DragSelection
	};

	void updateOutputs();

	void onModified(const pipeline::Modified&);

	void onPenDown(const gui::PenDown& signal);
	void onPenUp(const gui::PenUp& signal);
	void onPenMove(const gui::PenMove& signal);

	void onAddPage(const AddPage& signal);

	void anchorSelection();

	pipeline::Input<Document>   _initialDocument;
	pipeline::Input<PenMode>    _penMode;
	pipeline::Input<OsdRequest> _osdRequest;
	pipeline::Output<Document>  _document;
	pipeline::Output<Overlay>   _overlay;

	bool _penDown;

	Mode _mode;
	util::point<DocumentPrecision> _previousPosition;
	unsigned int                   _currentElement;

	bool _initialDocumentModified;

	boost::shared_ptr<Lasso> _lasso;

	signals::Slot<DocumentChangedArea> _documentChangedArea;
	signals::Slot<StrokePointAdded>    _strokePointAdded;
	signals::Slot<SelectionMoved>      _selectionMoved;

	signals::Slot<OverlayChangedArea>  _overlayChangedArea;
	signals::Slot<LassoPointAdded>     _lassoPointAdded;
};

#endif // YANTA_BACKEND_H__

