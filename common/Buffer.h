/*
 * Buffer.h
 *
 *  Created on: 2015-11-13
 *      Author: Max
 */

#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOG_BUFFER_LEN 8192
#define MAX_BUFFER_LEN 4096

typedef struct Buffer {
	int		len;
	char	buffer[MAX_BUFFER_LEN];

	Buffer() {
		Reset();
	}

	void Reset() {
		len = 0;
		memset(buffer, '\0', sizeof(buffer));
	}

} Buffer;

#endif /* BUFFER_H_ */
