#!/usr/bin/env sh

BUILD_FOLDER=$1
if [[ "$BUILD_FOLDER" == "" ]]
then
    echo "No build directory specified. assuming it is called 'build'" >&2
    BUILD_FOLDER="build"
fi
BUILD_OUT="./$BUILD_FOLDER/main"
if [[ ! -d "$BUILD_OUT" ]]
then
    echo "File $BUILD_OUT does not exist" >&2
    exit 1
fi

PERF_DATA="perf_data"
mkdir -p $PERF_DATA
TIMESTAMP=$(date '+%d:%m:%YT%H:%M:%S')
PERF_RECORD_OUTPUT="./$PERF_DATA/perf-$TIMESTAMP.data"
PERF_SCRIPT_OUT="./$PERF_DATA/out-$TIMESTAMP.perf"

echo "Profiling executable at $BUILD_OUT and generating flamegraph" >&2
perf record --output=$PERF_RECORD_OUTPUT -F 99 -g $BUILD_OUT 
perf script --input=$PERF_RECORD_OUTPUT > $PERF_SCRIPT_OUT

GEN_SVG_GRAPH=true
which stackcollapse-perf.pl 2>/dev/null >&2
if [[ "$?" -ne 0 ]]
then
    echo "stackcollapse-perf.pl not available in PATH" >&2
    GEN_SVG_GRAPH=false
fi

which flamegraph.pl 2>/dev/null >&2
if [[ "$?" -ne 0 ]]
then
    echo "flamegraph.pl not available in PATH" >&2
    GEN_SVG_GRAPH=false
fi

if [[ "$GEN_SVG_GRAPH" = true ]]
then
    # perl scripts come from here: https://github.com/brendangregg/flamegraph
    # this script expects them to be available in your PATH
    stackcollapse-perf.pl $PERF_SCRIPT_OUT | flamegraph.pl > ./$PERF_DATA/flamegraph-$TIMESTAMP.svg
else
    echo "Flamegraph not generated. Please generate it manually" >&2
fi

echo "Check perf_data directory for the results" >&2