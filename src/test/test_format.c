/*-
 * Copyright (c) 2015 UPLEX Nils Goroll Systemoptimierung
 * Copyright (c) 2015 Otto Gmbh & Co KG
 * All rights reserved
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

#include <string.h>
#include <math.h>

#include "vre.h"
#include "minunit.h"

#include "../varnishevent.h"
#include "../format.h"

#define NRECORDS 10
#define SHORT_STRING "foo bar baz quux"

int tests_run = 0;

static void
add_rec_chunk(tx_t *tx, logline_t *rec, chunk_t *chunk)
{
    VSTAILQ_INSERT_TAIL(&tx->lines, rec, linelist);
    rec->magic = LOGLINE_MAGIC;
    VSTAILQ_INIT(&rec->chunks);
    VSTAILQ_INSERT_TAIL(&rec->chunks, chunk, chunklist);
    chunk->magic = CHUNK_MAGIC;
    chunk->data = (char *) calloc(1, config.chunk_size);
}

static void
init_tx_rec_chunk(tx_t *tx, logline_t *rec, chunk_t *chunk)
{
    tx->magic = TX_MAGIC;
    VSTAILQ_INIT(&tx->lines);
    add_rec_chunk(tx, rec, chunk);
}

static void
set_record_data(logline_t *rec, chunk_t *chunk, const char *data,
                enum VSL_tag_e tag)
{
    rec->len = strlen(data);
    strcpy(chunk->data, data);
    if (tag != SLT__Bogus)
        rec->tag = tag;
}

/* N.B.: Always run the tests in this order */
static const char
*test_format_init(void)
{
    printf("... initializing format tests\n");

    CONF_Init();

    payload = VSB_new(NULL, NULL, DEFAULT_MAX_RECLEN + 1, VSB_FIXEDLEN);
    MAN(payload);

    return NULL;
}

static const char
*test_format_get_payload(void)
{
    logline_t rec;
    chunk_t chunk;

    printf("... testing get_payload()\n");

    rec.magic = LOGLINE_MAGIC;
    VSTAILQ_INIT(&rec.chunks);
    chunk.magic = CHUNK_MAGIC;
    chunk.data = (char *) calloc(1, config.chunk_size);
    MAN(chunk.data);

    /* Record with one chunk */
    rec.len = strlen(SHORT_STRING);
    strcpy(chunk.data, SHORT_STRING);
    VSTAILQ_INSERT_TAIL(&rec.chunks, &chunk, chunklist);
    get_payload(&rec);
    MASSERT(strcmp(VSB_data(payload), SHORT_STRING) == 0);

    /* Record with chunks that fill out shm_reclen */
    rec.len = config.max_reclen;
    int n = config.max_reclen;
    sprintf(chunk.data, "%0*d", config.chunk_size, 0);
    n -= config.chunk_size;
    while (n > 0) {
        int cp = n;
        if (cp > config.chunk_size)
            cp = config.chunk_size;
        chunk_t *c = (chunk_t *) malloc(sizeof(chunk_t));
        MAN(c);
        c->magic = CHUNK_MAGIC;
        c->data = (char *) calloc(1, config.chunk_size);
        sprintf(c->data, "%0*d", cp, 0);
        VSTAILQ_INSERT_TAIL(&rec.chunks, c, chunklist);
        n -= cp;
    }
    char *str = (char *) malloc(config.max_reclen);
    MAN(str);
    sprintf(str, "%0*d", config.max_reclen, 0);
    get_payload(&rec);
    MASSERT(strcmp(VSB_data(payload), str) == 0);

    /* Empty record */
    rec.len = 0;
    *chunk.data = '\0';
    get_payload(&rec);
    MASSERT(strlen(VSB_data(payload)) == 0);

    return NULL;
}

static const char
*test_format_get_tag(void)
{
    tx_t tx;
    logline_t recs[NRECORDS], *rec;

    printf("... testing get_tag()\n");

    tx.magic = TX_MAGIC;
    VSTAILQ_INIT(&tx.lines);
    for (int i = 0; i < NRECORDS; i++) {
        recs[i].magic = LOGLINE_MAGIC;
        recs[i].tag = SLT_ReqHeader;
        VSTAILQ_INSERT_TAIL(&tx.lines, &recs[i], linelist);
    }
    recs[NRECORDS / 2].tag = SLT_RespHeader;
    recs[NRECORDS - 1].tag = SLT_RespHeader;
    rec = get_tag(&tx, SLT_RespHeader);
    MASSERT(rec == &recs[NRECORDS - 1]);

    /* Record not found */
    recs[NRECORDS / 2].tag = SLT_ReqHeader;
    recs[NRECORDS - 1].tag = SLT_ReqHeader;
    rec = get_tag(&tx, SLT_RespHeader);
    MAZ(rec);

    /* Empty line list */
    VSTAILQ_INIT(&tx.lines);
    rec = get_tag(&tx, SLT_ReqHeader);
    MAZ(rec);

    return NULL;
}

static const char
*test_format_get_hdr(void)
{
    tx_t tx;
    logline_t recs[NRECORDS];
    chunk_t c[NRECORDS];
    char *hdr;

    printf("... testing get_hdr()\n");

    tx.magic = TX_MAGIC;
    VSTAILQ_INIT(&tx.lines);
    for (int i = 0; i < NRECORDS; i++) {
        recs[i].magic = LOGLINE_MAGIC;
        recs[i].tag = SLT_ReqHeader;
        recs[i].len = strlen("Bar: baz");
        VSTAILQ_INSERT_TAIL(&tx.lines, &recs[i], linelist);
        VSTAILQ_INIT(&recs[i].chunks);
        c[i].magic = CHUNK_MAGIC;
        c[i].data = (char *) calloc(1, config.chunk_size);
        strcpy(c[i].data, "Bar: baz");
        VSTAILQ_INSERT_TAIL(&recs[i].chunks, &c[i], chunklist);
    }
    recs[NRECORDS / 2].len = strlen("Foo: quux");
    strcpy(c[NRECORDS / 2].data, "Foo: quux");
    recs[NRECORDS - 1].len = strlen("Foo: wilco");
    strcpy(c[NRECORDS - 1].data, "Foo: wilco");
    hdr = get_hdr(&tx, SLT_ReqHeader, "Foo");
    MAN(hdr);
    MASSERT(strcmp(hdr, "wilco") == 0);

    /* Case-insensitive match */
    hdr = get_hdr(&tx, SLT_ReqHeader, "fOO");
    MAN(hdr);
    MASSERT(strcmp(hdr, "wilco") == 0);

    /* Ignore whitespace  */
    recs[NRECORDS - 1].len = strlen("  Foo  :  wilco");
    strcpy(c[NRECORDS - 1].data, "  Foo  :  wilco");
    hdr = get_hdr(&tx, SLT_ReqHeader, "Foo");
    MAN(hdr);
    MASSERT(strcmp(hdr, "wilco") == 0);

    /* Record not found */
    recs[NRECORDS / 2].tag = SLT_RespHeader;
    recs[NRECORDS - 1].tag = SLT_RespHeader;
    hdr = get_hdr(&tx, SLT_ReqHeader, "Foo");
    MAZ(hdr);

    /* Empty line list */
    VSTAILQ_INIT(&tx.lines);
    hdr = get_hdr(&tx, SLT_ReqHeader, "Foo");
    MAZ(hdr);

    return NULL;
}

static const char
*test_format_get_fld(void)
{
    char *fld, str[sizeof(SHORT_STRING)];

    printf("... testing get_fld()\n");

    strcpy(str, SHORT_STRING);

    fld = get_fld(str, 0);
    MAN(fld);
    MASSERT(strcmp(fld, "foo") == 0);

    fld = get_fld(str, 1);
    MAN(fld);
    MASSERT(strcmp(fld, "bar") == 0);

    fld = get_fld(str, 2);
    MAN(fld);
    MASSERT(strcmp(fld, "baz") == 0);

    fld = get_fld(str, 3);
    MAN(fld);
    MASSERT(strcmp(fld, "quux") == 0);

    fld = get_fld(str, 4);
    MAZ(fld);

    strcpy(str, "   ");
    fld = get_fld(str, 0);
    MAZ(fld);
    fld = get_fld(str, 1);
    MAZ(fld);
    fld = get_fld(str, 2);
    MAZ(fld);

    return NULL;
}

static const char
*test_format_get_rec_fld(void)
{
    logline_t rec;
    chunk_t chunk;
    char *fld;

    printf("... testing get_rec_fld()\n");

    rec.magic = LOGLINE_MAGIC;
    VSTAILQ_INIT(&rec.chunks);
    chunk.magic = CHUNK_MAGIC;
    chunk.data = (char *) calloc(1, config.chunk_size);
    MAN(chunk.data);
    rec.len = strlen(SHORT_STRING);
    strcpy(chunk.data, SHORT_STRING);
    VSTAILQ_INSERT_TAIL(&rec.chunks, &chunk, chunklist);

    fld = get_rec_fld(&rec, 0);
    MAN(fld);
    MASSERT(strcmp(fld, "foo") == 0);

    fld = get_rec_fld(&rec, 1);
    MAN(fld);
    MASSERT(strcmp(fld, "bar") == 0);

    fld = get_rec_fld(&rec, 2);
    MAN(fld);
    MASSERT(strcmp(fld, "baz") == 0);

    fld = get_rec_fld(&rec, 3);
    MAN(fld);
    MASSERT(strcmp(fld, "quux") == 0);

    fld = get_rec_fld(&rec, 4);
    MAZ(fld);

    rec.len = strlen("     ");
    strcpy(chunk.data, "     ");
    fld = get_rec_fld(&rec, 0);
    MAZ(fld);
    fld = get_rec_fld(&rec, 1);
    MAZ(fld);
    fld = get_rec_fld(&rec, 2);
    MAZ(fld);

    return NULL;
}

static const char
*test_format_get_tm(void)
{
#define T1 "Start: 1427743146.529143 0.000000 0.000000"
#define TIME 1427743146.529306
#define TX_TIME 1427744284.563984
    tx_t tx;
    logline_t recs[NRECORDS];
    chunk_t c[NRECORDS];
    double tm;

    printf("... testing get_tm()\n");

    tx.magic = TX_MAGIC;
    tx.t = TX_TIME;
    VSTAILQ_INIT(&tx.lines);
    for (int i = 0; i < NRECORDS; i++) {
        recs[i].magic = LOGLINE_MAGIC;
        recs[i].tag = SLT_ReqHeader;
        recs[i].len = strlen("Bar: baz");
        VSTAILQ_INSERT_TAIL(&tx.lines, &recs[i], linelist);
        VSTAILQ_INIT(&recs[i].chunks);
        c[i].magic = CHUNK_MAGIC;
        c[i].data = (char *) calloc(1, config.chunk_size);
        strcpy(c[i].data, "Bar: baz");
        VSTAILQ_INSERT_TAIL(&recs[i].chunks, &c[i], chunklist);
    }
    recs[NRECORDS / 2].tag = SLT_Timestamp;
    recs[NRECORDS / 2].len = strlen(T1);
    strcpy(c[NRECORDS / 2].data, T1);
    recs[NRECORDS - 1].tag = SLT_Timestamp;
    sprintf(c[NRECORDS - 1].data, "Start: %.6f 0.000000 0.000000", TIME);
    recs[NRECORDS - 1].len = strlen(c[NRECORDS - 1].data);
    tm = get_tm(&tx);
    MASSERT(fabs(tm - TIME) < 1e-6);

    /* Start timestamp not found, use the tx timestamp */
    recs[NRECORDS / 2].tag = SLT_ReqHeader;
    recs[NRECORDS - 1].tag = SLT_ReqHeader;
    tm = get_tm(&tx);
    MASSERT(fabs(tm - TX_TIME) < 1e-6);

    /* Empty line list */
    VSTAILQ_INIT(&tx.lines);
    tm = get_tm(&tx);
    MASSERT(fabs(tm - TX_TIME) < 1e-6);

    return NULL;
}

static const char
*test_format_H(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_H_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define PROTOCOL_PAYLOAD "HTTP/1.1"
    set_record_data(&rec, &chunk, PROTOCOL_PAYLOAD, SLT_ReqProtocol);
    format_H_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, PROTOCOL_PAYLOAD) == 0);
    MASSERT(len == strlen(PROTOCOL_PAYLOAD));

    rec.tag = SLT_BereqProtocol;
    format_H_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, PROTOCOL_PAYLOAD) == 0);
    MASSERT(len == strlen(PROTOCOL_PAYLOAD));

    return NULL;
}

static const char
*test_format_b(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_b_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define REQACCT_PAYLOAD "60 0 60 178 105 283"
    set_record_data(&rec, &chunk, REQACCT_PAYLOAD, SLT_ReqAcct);
    format_b_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "105") == 0);
    MASSERT(len == 3);

    rec.tag = SLT_BereqAcct;
    format_b_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "105") == 0);
    MASSERT(len == 3);

    return NULL;
}

static const char
*test_format_D(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_D_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define TS_RESP_PAYLOAD "Resp: 1427799478.166798 0.015963 0.000125"
    set_record_data(&rec, &chunk, TS_RESP_PAYLOAD, SLT_Timestamp);
    format_D_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "15963") == 0);
    MASSERT(len == 5);

#define TS_BERESP_PAYLOAD "BerespBody: 1427799478.166678 0.015703 0.000282"
    set_record_data(&rec, &chunk, TS_BERESP_PAYLOAD, SLT_Timestamp);
    format_D_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "15703") == 0);
    MASSERT(len == 5);

    return NULL;
}

static const char
*test_format_h(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_h_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define REQSTART_PAYLOAD "127.0.0.1 33544"
    set_record_data(&rec, &chunk, REQSTART_PAYLOAD, SLT_ReqStart);
    format_h_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "127.0.0.1") == 0);
    MASSERT(len == 9);

#define BACKEND_PAYLOAD "14 default default(127.0.0.1,,80)"
    set_record_data(&rec, &chunk, BACKEND_PAYLOAD, SLT_Backend);
    format_h_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "default(127.0.0.1,,80)") == 0);
    MASSERT(len == 22);

    return NULL;
}

static const char
*test_format_I(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_I_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

    set_record_data(&rec, &chunk, REQACCT_PAYLOAD, SLT_ReqAcct);
    format_I_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "60") == 0);
    MASSERT(len == 2);

    rec.tag = SLT_BereqAcct;
    format_I_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "283") == 0);
    MASSERT(len == 3);

#define PIPEACCT_PAYLOAD "60 60 178 105"
    set_record_data(&rec, &chunk, PIPEACCT_PAYLOAD, SLT_PipeAcct);
    format_I_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "178") == 0);
    MASSERT(len == 3);

    return NULL;
}

static const char
*test_format_m(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_m_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define REQMETHOD_PAYLOAD "GET"
    set_record_data(&rec, &chunk, REQMETHOD_PAYLOAD, SLT_ReqMethod);
    format_m_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET") == 0);
    MASSERT(len == 3);

    rec.tag = SLT_BereqMethod;
    format_m_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET") == 0);
    MASSERT(len == 3);

    return NULL;
}

static const char
*test_format_O(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_O_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

    set_record_data(&rec, &chunk, REQACCT_PAYLOAD, SLT_ReqAcct);
    format_O_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "283") == 0);
    MASSERT(len == 3);

    rec.tag = SLT_BereqAcct;
    format_O_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "60") == 0);
    MASSERT(len == 2);

    set_record_data(&rec, &chunk, PIPEACCT_PAYLOAD, SLT_PipeAcct);
    format_O_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "105") == 0);
    MASSERT(len == 3);

    return NULL;
}

static const char
*test_format_q(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_q_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define URL_QUERY_PAYLOAD "/foo?bar=baz&quux=wilco"
    set_record_data(&rec, &chunk, URL_QUERY_PAYLOAD, SLT_ReqURL);
    format_q_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "bar=baz&quux=wilco") == 0);
    MASSERT(len == 18);

    rec.tag = SLT_BereqURL;
    format_q_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "bar=baz&quux=wilco") == 0);
    MASSERT(len == 18);

#define URL_PAYLOAD "/foo"
    set_record_data(&rec, &chunk, URL_PAYLOAD, SLT_ReqURL);
    str = NULL;
    len = 0;
    format_q_client(&tx, NULL, SLT__Bogus, &str, &len);
    MAZ(str);
    MAZ(len);

    rec.tag = SLT_BereqURL;
    format_q_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MAZ(str);
    MAZ(len);

    return NULL;
}

static const char
*test_format_r(void)
{
    tx_t tx;
    logline_t rec_method, rec_host, rec_url, rec_proto;
    chunk_t chunk_method, chunk_host, chunk_url, chunk_proto;
    char *str;
    size_t len;

    printf("... testing format_r_*()\n");

    init_tx_rec_chunk(&tx, &rec_method, &chunk_method);
    MAN(chunk_method.data);
    add_rec_chunk(&tx, &rec_host, &chunk_host);
    MAN(chunk_host.data);
    add_rec_chunk(&tx, &rec_url, &chunk_url);
    MAN(chunk_url.data);
    add_rec_chunk(&tx, &rec_proto, &chunk_proto);
    MAN(chunk_proto.data);

    set_record_data(&rec_method, &chunk_method, "GET", SLT_ReqMethod);
    set_record_data(&rec_host, &chunk_host, "Host: www.foobar.com",
                    SLT_ReqHeader);
    set_record_data(&rec_url, &chunk_url, URL_PAYLOAD, SLT_ReqURL);
    set_record_data(&rec_proto, &chunk_proto, PROTOCOL_PAYLOAD,
                    SLT_ReqProtocol);
    format_r_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://www.foobar.com/foo HTTP/1.1") == 0);
    MASSERT(len == 38);

    rec_method.tag = SLT_BereqMethod;
    rec_host.tag = SLT_BereqHeader;
    rec_url.tag = SLT_BereqURL;
    rec_proto.tag = SLT_BereqProtocol;
    format_r_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://www.foobar.com/foo HTTP/1.1") == 0);
    MASSERT(len == 38);

    /* No method record */
    rec_method.tag = SLT__Bogus;
    rec_host.tag = SLT_ReqHeader;
    rec_url.tag = SLT_ReqURL;
    rec_proto.tag = SLT_ReqProtocol;
    format_r_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "- http://www.foobar.com/foo HTTP/1.1") == 0);
    MASSERT(len == 36);

    rec_host.tag = SLT_BereqHeader;
    rec_url.tag = SLT_BereqURL;
    rec_proto.tag = SLT_BereqProtocol;
    format_r_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "- http://www.foobar.com/foo HTTP/1.1") == 0);
    MASSERT(len == 36);

    /* No host header */
    rec_method.tag = SLT_ReqMethod;
    set_record_data(&rec_host, &chunk_host, "Foo: bar", SLT_ReqHeader);
    rec_url.tag = SLT_ReqURL;
    rec_proto.tag = SLT_ReqProtocol;
    format_r_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://localhost/foo HTTP/1.1") == 0);
    MASSERT(len == 33);

    rec_method.tag = SLT_BereqMethod;
    rec_host.tag = SLT_BereqHeader;
    rec_url.tag = SLT_BereqURL;
    rec_proto.tag = SLT_BereqProtocol;
    format_r_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://localhost/foo HTTP/1.1") == 0);
    MASSERT(len == 33);

    /* No header record */
    rec_method.tag = SLT_ReqMethod;
    rec_host.tag = SLT__Bogus;
    rec_url.tag = SLT_ReqURL;
    rec_proto.tag = SLT_ReqProtocol;
    format_r_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://localhost/foo HTTP/1.1") == 0);
    MASSERT(len == 33);

    rec_method.tag = SLT_BereqMethod;
    rec_url.tag = SLT_BereqURL;
    rec_proto.tag = SLT_BereqProtocol;
    format_r_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://localhost/foo HTTP/1.1") == 0);
    MASSERT(len == 33);

    /* URL record empty */
    set_record_data(&rec_host, &chunk_host, "Host: www.foobar.com",
                    SLT_ReqHeader);
    rec_method.tag = SLT_ReqMethod;
    rec_url.tag = SLT_ReqURL;
    rec_url.len = 0;
    rec_proto.tag = SLT_ReqProtocol;
    format_r_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://www.foobar.com- HTTP/1.1") == 0);
    MASSERT(len == 35);

    rec_method.tag = SLT_BereqMethod;
    rec_host.tag = SLT_BereqHeader;
    rec_url.tag = SLT_BereqURL;
    rec_proto.tag = SLT_BereqProtocol;
    format_r_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://www.foobar.com- HTTP/1.1") == 0);
    MASSERT(len == 35);

    /* Proto record empty */
    rec_method.tag = SLT_ReqMethod;
    rec_host.tag = SLT_ReqHeader;
    rec_url.tag = SLT_ReqURL;
    rec_url.len = strlen(URL_PAYLOAD);
    rec_proto.tag = SLT_ReqProtocol;
    rec_proto.len = 0;
    format_r_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://www.foobar.com/foo HTTP/1.0") == 0);
    MASSERT(len == 38);

    rec_method.tag = SLT_BereqMethod;
    rec_host.tag = SLT_BereqHeader;
    rec_url.tag = SLT_BereqURL;
    rec_proto.tag = SLT_BereqProtocol;
    format_r_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "GET http://www.foobar.com/foo HTTP/1.0") == 0);
    MASSERT(len == 38);

    return NULL;
}

static const char
*test_format_s(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_s_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define STATUS_PAYLOAD "200"
    set_record_data(&rec, &chunk, STATUS_PAYLOAD, SLT_RespStatus);
    format_s_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, STATUS_PAYLOAD) == 0);
    MASSERT(len == 3);

    rec.tag = SLT_BerespStatus;
    format_s_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, STATUS_PAYLOAD) == 0);
    MASSERT(len == 3);

    return NULL;
}

static const char
*test_format_t(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    const char *error;
    size_t len;
    vre_t *time_re;
    int n;

    printf("... testing format_t()\n");

#define HTTP_DATA_REGEX \
    "^\\[\\d\\d/Mar/2015:\\d\\d:\\d\\d:\\d\\d [+-]\\d{4}\\]$"

    time_re = VRE_compile(HTTP_DATA_REGEX, 0, &error, &n);
    VMASSERT(time_re != NULL,
             "Error compiling '" HTTP_DATA_REGEX "': %s (offset %d)",
             error, n);

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

    set_record_data(&rec, &chunk, T1, SLT_Timestamp);
    format_t(&tx, NULL, SLT__Bogus, &str, &len);
    n = VRE_exec(time_re, str, strlen(str), 0, 0, NULL, 0, NULL);
    VMASSERT(n > 0, "'%s' does not match '" HTTP_DATA_REGEX "', "
             "return code = %d", str, n);
    MASSERT(len == 28);

    return NULL;
}

static const char
*test_format_T(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_T_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

    set_record_data(&rec, &chunk, TS_RESP_PAYLOAD, SLT_Timestamp);
    format_T_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "0") == 0);
    MASSERT(len == 1);

    set_record_data(&rec, &chunk, TS_BERESP_PAYLOAD, SLT_Timestamp);
    format_T_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "0") == 0);
    MASSERT(len == 1);

    return NULL;
}

static const char
*test_format_U(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_U_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

    set_record_data(&rec, &chunk, URL_QUERY_PAYLOAD, SLT_ReqURL);
    format_U_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "/foo") == 0);
    MASSERT(len == 4);

    rec.tag = SLT_BereqURL;
    format_U_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "/foo") == 0);
    MASSERT(len == 4);

    set_record_data(&rec, &chunk, URL_PAYLOAD, SLT_ReqURL);
    format_U_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "/foo") == 0);
    MASSERT(len == 4);

    rec.tag = SLT_BereqURL;
    format_U_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "/foo") == 0);
    MASSERT(len == 4);

    return NULL;
}

static const char
*test_format_u(void)
{
    tx_t tx;
    logline_t rec;
    chunk_t chunk;
    char *str;
    size_t len;

    printf("... testing format_u_*()\n");

    init_tx_rec_chunk(&tx, &rec, &chunk);
    MAN(chunk.data);

#define BASIC_AUTH_PAYLOAD "Authorization: Basic dmFybmlzaDo0ZXZlcg=="
    set_record_data(&rec, &chunk, BASIC_AUTH_PAYLOAD, SLT_ReqHeader);
    format_u_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "varnish") == 0);
    MASSERT(len == 7);

    rec.tag = SLT_BereqHeader;
    format_u_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "varnish") == 0);
    MASSERT(len == 7);

    /* No header record */
    rec.tag = SLT__Bogus;
    format_u_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "-") == 0);
    MASSERT(len == 1);

    format_u_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "-") == 0);
    MASSERT(len == 1);

    /* No auth header */
    rec.tag = SLT_ReqHeader;
    rec.len = 0;
    format_u_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "-") == 0);
    MASSERT(len == 1);

    rec.tag = SLT_BereqHeader;
    format_u_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "-") == 0);
    MASSERT(len == 1);

    /* No basic auth header
     * Not a real example of a digest auth header, but kept short, so
     * that we can test with only one chunk.
     */
#define DIGEST_AUTH_PAYLOAD "Authorization: Digest username=\"Mufasa\", realm=\"realm@host.com\""
    set_record_data(&rec, &chunk, DIGEST_AUTH_PAYLOAD, SLT_ReqHeader);
    format_u_client(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "-") == 0);
    MASSERT(len == 1);

    rec.tag = SLT_BereqHeader;
    format_u_backend(&tx, NULL, SLT__Bogus, &str, &len);
    MASSERT(strcmp(str, "-") == 0);
    MASSERT(len == 1);

    return NULL;
}

static const char
*all_tests(void)
{
    mu_run_test(test_format_init);
    mu_run_test(test_format_get_payload);
    mu_run_test(test_format_get_tag);
    mu_run_test(test_format_get_hdr);
    mu_run_test(test_format_get_fld);
    mu_run_test(test_format_get_rec_fld);
    mu_run_test(test_format_get_tm);
    mu_run_test(test_format_b);
    mu_run_test(test_format_D);
    mu_run_test(test_format_H);
    mu_run_test(test_format_h);
    mu_run_test(test_format_I);
    mu_run_test(test_format_m);
    mu_run_test(test_format_O);
    mu_run_test(test_format_q);
    mu_run_test(test_format_r);
    mu_run_test(test_format_s);
    mu_run_test(test_format_t);
    mu_run_test(test_format_T);
    mu_run_test(test_format_U);
    mu_run_test(test_format_u);

    return NULL;
}

TEST_RUNNER
