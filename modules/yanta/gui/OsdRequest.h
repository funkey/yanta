#ifndef YANTA_OSD_REQUEST_H__
#define YANTA_OSD_REQUEST_H__

#include <pipeline/Data.h>

/**
 * A lightweight class to establich a connection between the Osd and the 
 * Backend, used to send signals like AddPage.
 */
class OsdRequest : public pipeline::Data {};

#endif // YANTA_OSD_REQUEST_H__

