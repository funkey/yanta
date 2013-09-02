#ifndef YANTA_OSD_SIGNALS_H__
#define YANTA_OSD_SIGNALS_H__

#include <signals/Signal.h>

/**
 * Base class for all OSD signals.
 */
class OsdSignal : public signals::Signal {};

/**
 * Request the handling of an 'add' event.
 */
class Add : public OsdSignal {};

#endif // YANTA_OSD_SIGNALS_H__

