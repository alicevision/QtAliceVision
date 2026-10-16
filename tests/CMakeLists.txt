# QML tests, run against the installed plugins: `cmake --install` before `ctest`.
# They need a display with OpenGL (use xvfb-run or a virtual X server on a headless machine).
find_program(QMLTESTRUNNER qmltestrunner PATHS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}" REQUIRED)

add_test(NAME qmlTests
    COMMAND ${QMLTESTRUNNER} -input "${CMAKE_CURRENT_SOURCE_DIR}" -import "${CMAKE_INSTALL_PREFIX}/qml")
