SCRIPT_DIR=$(dirname $0)
REPO_DIR=$SCRIPT_DIR/..

pushd $REPO_DIR
    clang-format -i src/*.cpp src/*.h
popd
