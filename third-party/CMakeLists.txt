set(BUILD_SHARED_LIBS OFF)

set(
  THIRDPARTY_TOOLCHAIN_FILE
  "${CMAKE_CURRENT_BINARY_DIR}/toolchain.cmake"
)
configure_file(
  toolchain.cmake.in
  "${THIRDPARTY_TOOLCHAIN_FILE}"
  @ONLY
)

include(detours.cmake)
include(cppwinrt.cmake)
include(directxtk.cmake)
include(geographiclib.cmake)
include(json.cmake)
include(libjpeg-turbo.cmake)
include(lualib.cmake)
include(oculus-sdk.cmake)
include(out_ptr.cmake)
include(openvr.cmake)
include(otd-ipc.cmake)
include(openxr.cmake)
include(openxr-layer-scripts.cmake)
include(semver.cmake)
include(system.cmake)
include(vulkan-headers.cmake)
include(wintab.cmake)
include(wmm.cmake)
include(vrperfkit.cmake)
include(zlib.cmake)

include(libzip.cmake)
include(qpdf.cmake)
include(windowsappsdk.cmake)
