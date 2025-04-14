#ifndef INTERNAL_BUFFER_H
#define INTERNAL_BUFFER_H

#include <communication/comm_interface.h>

// Encodes data and inserts it into the internal buffer
int encode_and_insert(PulgaRingBuffer *buffer, CommunicationUnit data_unit, enum EncodingLevel encoding);

#endif /* INTERNAL_BUFFER_H */