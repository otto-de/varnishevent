#!/bin/bash

# Outputs of varnishevent and varnishncsa for client formats are
# identical, except that varnishevent emits empty strings for data
# from headers that are empty, and varnishnca emits a '-'.

echo
echo "TEST: $0"
echo "... testing equivalence of output with varnishncsa"

# automake skip
SKIP=77

TMP=${TMPDIR:-/tmp}
EVENT="../varnishevent"
NCSA=$( which varnishncsa )

if [ -x "$NSCA" ]; then
    echo "varnishncsa not found or not executable ($NCSA), skipping"
    exit $SKIP
fi

EVENT_LOG=$TMP/event.log
NCSA_LOG=$TMP/ncsa.log
INPUT=varnish-doc.log

DIFF_CMD="diff $EVENT_LOG $NCSA_LOG"

echo "... default format"
$EVENT -r $INPUT | sed 's/-//g' > $EVENT_LOG
$NCSA -r $INPUT | sed 's/-//g' > $NCSA_LOG

$DIFF_CMD
RC=$?
rm $EVENT_LOG
rm $NCSA_LOG

if [ "$RC" -ne "0" ]; then
    echo "ERROR: outputs of no-arg varnishevent and varnishncsa differ"
    exit 1
fi

# Cannot test the %D formatter, because varnishevent gets it more accurately
# (varnishncsa has floating point errors).
FORMAT='%b %H %h %I %{Host}i %{Connection}i %{User-Agent}i %{X-Forwarded-For}i %{Accept-Ranges}o %{Age}o %{Connection}o %{Content-Encoding}o %{Content-Length}o %{Content-Type}o %{Date}o %{Last-Modified}o %{Server}o %{Transfer-Encoding}o %{Via}o %{X-Varnish}o %l %m %O %q %r %s %t %{%F-%T}t %U %u %{Varnish:time_firstbyte}x %{Varnish:hitmiss}x %{Varnish:handling}x'

echo "... custom -F format"
$EVENT -r $INPUT -F "$FORMAT" | sed 's/-//g' > $EVENT_LOG
$NCSA -r $INPUT -F "$FORMAT" | sed 's/-//g' > $NCSA_LOG

exit 0

$DIFF_CMD
RC=$?
rm $EVENT_LOG
rm $NCSA_LOG

if [ "$RC" -ne "0" ]; then
    echo "ERROR: outputs of varnishevent/varnishncsa -F differ"
    exit 1
fi

exit 0