#!/bin/bash

echo
echo "TEST: $0"
echo '... testing output and log against known checksums'

# Ensure that the local time date formatters produce the same output
# wherever the test is run.
export TZ=UTC

LOG=test.log
OUT=output.log

rm -f $LOG $OUT

../varnishevent -f varnishevent.conf -r varnish-4.1.0-doc.log -w $OUT -v

CKSUM=$( cksum $OUT )
if [ "$CKSUM" != "54743924 442153 $OUT" ]; then
    echo "ERROR: Regression test log output incorrect cksum: $CKSUM"
    exit 1
fi

# sed removes the version/revision from the "initializing" line.
# grep removes logs about table allocations and by the threads about
# free lists, which are not relevant to the regression, and are not
# predictable from one run to the next.
CKSUM=$( sed -e 's/\(initializing\) \(.*\)/\1/' $LOG | egrep -v 'Writer: returned|Reader: took|^DEBUG: Allocating' | cksum )

if [ "$CKSUM" != '1109151867 836995' ]; then
    echo "ERROR: Regression test varnishevent log incorrect cksum: $CKSUM"
    exit 1
fi

exit 0
