#ifndef YANTA_OSD_SIGNALS_H__
#define YANTA_OSD_SIGNALS_H__

#include <signals/Signal.h>

/**
 * Base class for all OSD signals.
 */
class OsdSignal : public signals::Signal {};

/**
 * Request the creation of a new page.
 */
class AddPage : public OsdSignal {};

#endif // YANTA_OSD_SIGNALS_H__

