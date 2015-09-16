/*
 * base64.h
 *
 *  Created on: Mar 6, 2015
 *      Author: reboot
 */

#ifndef SRC_BASE64_H_
#define SRC_BASE64_H_

int
base64_encode (unsigned char *source, unsigned int sourcelen, char *target,
	       unsigned int targetlen);
unsigned int
base64_decode (char *source, unsigned char *target, unsigned int targetlen);

#endif /* SRC_BASE64_H_ */
