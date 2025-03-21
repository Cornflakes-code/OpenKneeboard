set(
  QPDF_DEPENDENCIES
  ThirdParty::LibJpeg ThirdParty::ZLib
)

ExternalProject_Add(
  qpdfBuild
  URL "https://github.com/qpdf/qpdf/releases/download/v11.6.3/qpdf-11.6.3.tar.gz"
  URL_HASH "SHA256=C394B1B0CFF4CD9D13B0F5E16BDF3CF54DA424DC434F9D40264B7FE67ACD90BC"
  CMAKE_ARGS
  "-DCMAKE_TOOLCHAIN_FILE=${THIRDPARTY_TOOLCHAIN_FILE}"
  "-DREQUIRE_CRYPTO_NATIVE=ON"
  "-DUSE_IMPLICIT_CRYPTO=OFF"
  "-DBUILD_SHARED_LIBS=OFF"
  "-DLIBJPEG_H_PATH=$<TARGET_PROPERTY:ThirdParty::LibJpeg,INTERFACE_INCLUDE_DIRECTORIES>"
  "-DLIBJPEG_LIB_PATH=$<TARGET_PROPERTY:ThirdParty::LibJpeg,INTERFACE_LINK_LIBRARIES>"
  "-DZLIB_H_PATH=$<TARGET_PROPERTY:ThirdParty::ZLib,INTERFACE_INCLUDE_DIRECTORIES>"
  "-DZLIB_LIB_PATH=$<TARGET_PROPERTY:ThirdParty::ZLib,INTERFACE_LINK_LIBRARIES>"
  BUILD_COMMAND
  "${CMAKE_COMMAND}"
  --build .
  --config "$<CONFIG>"
  --target "libqpdf"
  --parallel
  --
  /p:CL_MPCount=
  /p:UseMultiToolTask=true
  /p:EnforceProcessCountAcrossBuilds=true
  INSTALL_COMMAND
  "${CMAKE_COMMAND}"
  --install .
  --prefix "<INSTALL_DIR>/$<CONFIG>"
  --config "$<CONFIG>"
  --component "dev"
  DEPENDS ${QPDF_DEPENDENCIES}
  EXCLUDE_FROM_ALL
)

ExternalProject_Get_property(qpdfBuild SOURCE_DIR)
ExternalProject_Get_property(qpdfBuild INSTALL_DIR)
add_library(libqpdf INTERFACE)
add_dependencies(libqpdf INTERFACE qpdfBuild)

target_link_libraries(
  libqpdf
  INTERFACE
  "${INSTALL_DIR}/$<CONFIG>/lib/qpdf.lib"
  ${QPDF_DEPENDENCIES}
)
target_include_directories(
  libqpdf
  INTERFACE
  "${INSTALL_DIR}/$<CONFIG>/include"
)
target_compile_definitions(
  libqpdf
  INTERFACE
  POINTERHOLDER_TRANSITION=4
)
add_library(ThirdParty::QPDF ALIAS libqpdf)

install(
  FILES "${SOURCE_DIR}/NOTICE.md"
  TYPE DOC
  RENAME "LICENSE-ThirdParty-QPDF.txt"
)
