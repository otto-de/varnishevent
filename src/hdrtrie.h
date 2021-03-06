/*-
 * Copyright (c) 2015 UPLEX Nils Goroll Systemoptimierung
 * Copyright (c) 2015 Otto Gmbh & Co KG
 * All rights reserved.
 * Use only with permission
 *
 * Author: Geoffrey Simmons <geoffrey.simmons@uplex.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef HDRTRIE_H_INCLUDED
#define HDRTRIE_H_INCLUDED

#include "vdef.h"
#include "vsb.h"

struct hdrt_node {
    unsigned		magic;
#define HDRT_NODE_MAGIC 0x970ec029
    char		*str;
    struct hdrt_node	**next;
    int			idx;
};

int HDR_FindIdx(struct hdrt_node *hdrt, const char *hdr);
struct hdrt_node *HDR_InsertIdx(struct hdrt_node *hdrt, const char *hdr,
                                int idx);
int HDR_N(struct hdrt_node *hdrt);
void HDR_List(struct hdrt_node *hdrt, struct vsb *sb);
void HDR_Fini(struct hdrt_node *hdrt);

#endif
