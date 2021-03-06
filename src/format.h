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

#include "varnishevent.h"

typedef struct arg_t {
    char *name;
    enum VSL_tag_e tag;
    int fld;
    int hdr_idx;
} arg_t;

typedef void formatter_f(const tx_t *tx, const arg_t *args, char **s,
                         size_t *len);

int hidx[MAX_VSL_TAG];

char *get_payload(const rec_t *rec);
rec_t *get_tag(const tx_t *tx, enum VSL_tag_e tag);
char *get_hdr(const tx_t *tx, enum VSL_tag_e tag, int hdr_idx);
char *get_fld(char *str, int n, size_t *len);
char *get_rec_fld(const rec_t *rec, int n, size_t *len);

formatter_f format_b_client;
formatter_f format_b_backend;

formatter_f format_D_client;
formatter_f format_D_backend;

formatter_f format_H_client;
formatter_f format_H_backend;

formatter_f format_h_client;
formatter_f format_h_backend;

formatter_f format_I_client;
formatter_f format_I_backend;

formatter_f format_m_client;
formatter_f format_m_backend;

formatter_f format_O_client;
formatter_f format_O_backend;

formatter_f format_q_client;
formatter_f format_q_backend;

formatter_f format_r_client;
formatter_f format_r_backend;

formatter_f format_s_client;
formatter_f format_s_backend;

formatter_f format_t;

formatter_f format_T_client;
formatter_f format_T_backend;

formatter_f format_U_client;
formatter_f format_U_backend;

formatter_f format_u_client;
formatter_f format_u_backend;

formatter_f format_Xi_client;
formatter_f format_Xi_backend;
formatter_f format_Xo_client;
formatter_f format_Xo_backend;

formatter_f format_Xt;

formatter_f format_Xttfb_client;
formatter_f format_Xttfb_backend;

formatter_f format_VCL_disp;

formatter_f format_VCL_Log;

formatter_f format_SLT;

formatter_f format_vxid;
formatter_f format_pvxid;
