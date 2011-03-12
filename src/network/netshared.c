
/* Copyright (C) 2011 Rafael Ostertag 
 *
 * This file is part of agentsmith.
 *
 * agentsmith is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * agentsmith is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * agentsmith.  If not, see <http://www.gnu.org/licenses/>.
 */

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "globals.h"
#include "netshared.h"
#include "output.h"
#include "records.h"

#ifndef WORDS_BIGENDIAN
struct ___tmp_conv_struct {
    char      __byte__[8];
};
#endif

/**
 * Prepares the buffer for the command to be sent over the wire. It takes care
 * of translating the integer values to network presentation.
 *
 * @param command the command as defined in netshared.h
 *
 * @param rec pointer to the host record to be sent over the wire
 *
 * @param buffsize the buffer size. It is expected to be exactly of size
 * REMOTE_COMMAND_SIZE as found in globals.h
 *
 * @return The amount of bytes used in the buffer, which should be exactly
 * REMOTE_COMMAND_SIZE bytes
 */
size_t
net_command_to_buff(uint32_t command, const hostrecord_t *rec, char *buff,
		    size_t buffsize) {
    int       pos = 0;
    uint64_t  val64;
    uint32_t  val32;

    assert(rec != NULL);
    assert(buff != NULL);
    assert(buffsize == REMOTE_COMMAND_SIZE);

    val32 = htonl(command);
    memcpy(buff + pos, &val32, sizeof (uint32_t));
    pos += sizeof (uint32_t);

    memcpy(buff + pos, rec->origin, IPADDR_SIZE);
    pos += IPADDR_SIZE;

    memcpy(buff + pos, rec->ipaddr, IPADDR_SIZE);
    pos += IPADDR_SIZE;

    val64 = htonll(rec->firstseen);
    memcpy(buff + pos, &val64, sizeof (int64_t));
    pos += sizeof (int64_t);

    val64 = htonll(rec->lastseen);
    memcpy(buff + pos, &val64, sizeof (int64_t));
    pos += sizeof (int64_t);

    val64 = htonll(rec->purge_after);
    memcpy(buff + pos, &val64, sizeof (int64_t));
    pos += sizeof (int64_t);

    val64 = htonll(rec->time_interval);
    memcpy(buff + pos, &val64, sizeof (int64_t));
    pos += sizeof (int64_t);

    val32 = htonl(rec->occurrences);
    memcpy(buff + pos, &val32, sizeof (int32_t));
    pos += sizeof (int32_t);

    val32 = htonl(rec->action_threshold);
    memcpy(buff + pos, &val32, sizeof (int32_t));
    pos += sizeof (int32_t);

    val32 = htonl(rec->remove);
    memcpy(buff + pos, &val32, sizeof (int32_t));
    pos += sizeof (int32_t);

    val32 = htonl(rec->processed);
    memcpy(buff + pos, &val32, sizeof (int32_t));
    pos += sizeof (int32_t);

    assert(pos == buffsize);
    assert(pos == REMOTE_COMMAND_SIZE);

    return pos;
}

/**
 * Creates the hostrecord_t from a remote command sent over the wire. It takes
 * care of translating the integer values to network presentation.
 *
 * @param buff pointer to the buffer as received over the wire
 *
 * @param command pointer to the command as defined in netshared.h
 *
 * @param rec pointer to the record that will be filled with the values
 * received.
 *
 * @return The amount of bytes used in the buffer, which should be exactly
 * REMOTE_COMMAND_SIZE bytes
 */
size_t
net_buff_to_command(const char *buff, uint32_t * command, hostrecord_t *rec) {
    int       pos = 0;
    uint64_t  val64;
    uint32_t  val32;

    assert(rec != NULL);
    assert(buff != NULL);
    assert(command != NULL);

    memcpy(&val32, buff + pos, sizeof (uint32_t));
    *command = ntohl(val32);
    pos += sizeof (uint32_t);

    memcpy(rec->origin, buff + pos, IPADDR_SIZE);
    pos += IPADDR_SIZE;

    memcpy(rec->ipaddr, buff + pos, IPADDR_SIZE);
    pos += IPADDR_SIZE;

    memcpy(&val64, buff + pos, sizeof (int64_t));
    rec->firstseen = ntohll(val64);
    pos += sizeof (int64_t);

    memcpy(&val64, buff + pos, sizeof (int64_t));
    rec->lastseen = ntohll(val64);
    pos += sizeof (int64_t);

    memcpy(&val64, buff + pos, sizeof (int64_t));
    rec->purge_after = ntohll(val64);
    pos += sizeof (int64_t);

    memcpy(&val64, buff + pos, sizeof (int64_t));
    rec->time_interval = ntohll(val64);
    pos += sizeof (int64_t);

    memcpy(&val32, buff + pos, sizeof (int32_t));
    rec->occurrences = ntohl(val32);
    pos += sizeof (int32_t);

    memcpy(&val32, buff + pos, sizeof (int32_t));
    rec->action_threshold = ntohl(val32);
    pos += sizeof (int32_t);

    memcpy(&val32, buff + pos, sizeof (int32_t));
    rec->remove = ntohl(val32);
    pos += sizeof (int32_t);

    memcpy(&val32, buff + pos, sizeof (int32_t));
    rec->processed = ntohl(val32);
    pos += sizeof (int32_t);

    assert(pos == REMOTE_COMMAND_SIZE);

    return pos;
}

ssize_t
net_read(int fildes, void *buf, size_t nbyte, int *err) {
    size_t    nleft;
    ssize_t   nread;
    char     *ptr;
    int       _err = 0;

    assert(buf != NULL);

    if (err == NULL)
	err = &_err;		/* To avoid NULL ptr deref */

    ptr = buf;
    nleft = nbyte;
    while (nleft > 0) {
	if ((nread = read(fildes, ptr, nleft)) < 0) {
	    *err = errno;
	    if (*err == EINTR)
		nread = 0;
	    else
		return RETVAL_ERR;
	} else if (nread == 0)
	    break;		/* EOF */

	nleft -= nread;
	ptr += nread;
    }
    return nbyte - nleft;
}

ssize_t
net_write(int fildes, const void *buf, size_t nbyte, int *err) {
    size_t    nleft;
    ssize_t   nwritten;
    const char *ptr;
    int       _err = 0;

    assert(buf != NULL);

    if (err == NULL)
	err = &_err;		/* To avoid NULL ptr deref */

    ptr = buf;
    nleft = nbyte;
    while (nleft > 0) {
	if ((nwritten = write(fildes, ptr, nleft)) <= 0) {
	    *err = errno;
	    if (*err == EINTR)
		nwritten = 0;
	    else
		return RETVAL_ERR;
	}

	nleft -= nwritten;
	ptr += nwritten;
    }

    return nbyte;
}

#ifndef WORDS_BIGENDIAN
uint64_t
htonll(uint64_t a) {
    struct ___tmp_conv_struct *sa;
    struct ___tmp_conv_struct *sb;
    uint64_t  b;
    register int i;

    sa = (struct ___tmp_conv_struct *) &a;
    sb = (struct ___tmp_conv_struct *) &b;
    for (i = 0; i < 8; i++)
	sb->__byte__[i] = sa->__byte__[7 - i];

    return b;
}

uint64_t
ntohll(uint64_t a) {
    return htonll(a);
}
#endif

#ifdef DEBUG
void
__dbg_dump_host_record(hostrecord_t *ptr) {
    char     *format =
	"IP Addr %s origin %s first seen %s, last seen %s, occurrences %i, to be removed %i, processed %i";
#ifdef HAVE_CTIME_R
#ifdef DEBUG
#warning "++++ Using ctime_r() ++++"
#endif
#define TIMEBUFSIZE 128
    char      timebuff1[TIMEBUFSIZE], timebuff2[TIMEBUFSIZE];
#else
#ifdef DEBUG
#warning "++++ Using ctime() ++++"
#endif
#endif
    if (ptr == NULL) {
	out_dbg("__dbg_dump_host_record(): NULL pointer");
	return;
    }
#ifdef HAVE_CTIME_R
    ctime_r(&(ptr->firstseen), timebuff1);
    ctime_r(&(ptr->lastseen), timebuff2);
    /*
     * Get rid of the newlines 
     */
    timebuff1[TIMEBUFSIZE - 1] = '\0';
    timebuff2[TIMEBUFSIZE - 1] = '\0';

    timebuff1[strlen(timebuff1) - 1] = '\0';
    timebuff2[strlen(timebuff2) - 1] = '\0';
    out_dbg(format,
	    ptr->ipaddr,
	    ptr->origin,
	    timebuff1,
	    timebuff2, ptr->occurrences, ptr->remove, ptr->processed);
#else
    out_msg(format,
	    ptr->ipaddr,
	    ctime(ptr->firstseen),
	    ctime(ptr->lastseen),
	    ptr->occurrences, ptr->remove, ptr->processed);
#endif
}
#endif
