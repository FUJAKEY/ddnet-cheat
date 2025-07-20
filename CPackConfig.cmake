# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_ARCHIVE_COMPONENT_INSTALL "ON")
set(CPACK_ARCHIVE_PORTABLE_FILE_NAME "DDNet-19.3-linux_aarch64")
set(CPACK_BUILD_SOURCE_DIRS "/home/userland/v1/v2/sandbox;/home/userland/v1/v2/sandbox")
set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_COMPONENTS_ALL "portable")
set(CPACK_COMPONENTS_ALL_SET_BY_USER "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "/usr/share/cmake-3.18/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "DDNet built using CMake")
set(CPACK_FILES_TMP "license.txt;storage.cfg;other/config_directory.sh;data/shader/vulkan/prim.frag.spv;data/shader/vulkan/prim_textured.frag.spv;data/shader/vulkan/prim.vert.spv;data/shader/vulkan/prim_textured.vert.spv;data/shader/vulkan/prim3d.frag.spv;data/shader/vulkan/prim3d_textured.frag.spv;data/shader/vulkan/prim3d.vert.spv;data/shader/vulkan/prim3d_textured.vert.spv;data/shader/vulkan/text.frag.spv;data/shader/vulkan/text.vert.spv;data/shader/vulkan/primex.frag.spv;data/shader/vulkan/primex.vert.spv;data/shader/vulkan/primex_rotationless.frag.spv;data/shader/vulkan/primex_rotationless.vert.spv;data/shader/vulkan/primex_tex.frag.spv;data/shader/vulkan/primex_tex.vert.spv;data/shader/vulkan/primex_tex_rotationless.frag.spv;data/shader/vulkan/primex_tex_rotationless.vert.spv;data/shader/vulkan/spritemulti.frag.spv;data/shader/vulkan/spritemulti.vert.spv;data/shader/vulkan/spritemulti_push.frag.spv;data/shader/vulkan/spritemulti_push.vert.spv;data/shader/vulkan/tile.frag.spv;data/shader/vulkan/tile.vert.spv;data/shader/vulkan/tile_textured.frag.spv;data/shader/vulkan/tile_textured.vert.spv;data/shader/vulkan/tile_border.frag.spv;data/shader/vulkan/tile_border.vert.spv;data/shader/vulkan/tile_border_textured.frag.spv;data/shader/vulkan/tile_border_textured.vert.spv;data/shader/vulkan/quad.frag.spv;data/shader/vulkan/quad.vert.spv;data/shader/vulkan/quad_push.frag.spv;data/shader/vulkan/quad_push.vert.spv;data/shader/vulkan/quad_textured.frag.spv;data/shader/vulkan/quad_textured.vert.spv;data/shader/vulkan/quad_push_textured.frag.spv;data/shader/vulkan/quad_push_textured.vert.spv")
set(CPACK_GENERATOR "TGZ;TXZ")
set(CPACK_INSTALL_CMAKE_PROJECTS "/home/userland/v1/v2/sandbox;DDNet;ALL;/")
set(CPACK_INSTALL_PREFIX "/usr/local")
set(CPACK_MODULE_PATH "/home/userland/v1/v2/sandbox/cmake")
set(CPACK_NSIS_DISPLAY_NAME "DDNet 19.3")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_PACKAGE_NAME "DDNet 19.3")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OUTPUT_CONFIG_FILE "/home/userland/v1/v2/sandbox/CPackConfig.cmake")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/usr/share/cmake-3.18/Templates/CPack.GenericDescription.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "DDNet built using CMake")
set(CPACK_PACKAGE_FILE_NAME "DDNet-19.3-linux_aarch64")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "DDNet 19.3")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "DDNet 19.3")
set(CPACK_PACKAGE_NAME "DDNet")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Humanity")
set(CPACK_PACKAGE_VERSION "19.3")
set(CPACK_PACKAGE_VERSION_MAJOR "19")
set(CPACK_PACKAGE_VERSION_MINOR "3")
set(CPACK_RESOURCE_FILE_LICENSE "/usr/share/cmake-3.18/Templates/CPack.GenericLicense.txt")
set(CPACK_RESOURCE_FILE_README "/usr/share/cmake-3.18/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "/usr/share/cmake-3.18/Templates/CPack.GenericWelcome.txt")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_GENERATOR "ZIP;TGZ;TBZ2;TXZ")
set(CPACK_SOURCE_IGNORE_FILES "\\.pyc$;/\\.git;/__pycache__/;/home/userland/v1/v2/sandbox/([^CRcdddllmosssuv]|$);/home/userland/v1/v2/sandbox/C([^M]|$);/home/userland/v1/v2/sandbox/R([^E]|$);/home/userland/v1/v2/sandbox/c([^m]|$);/home/userland/v1/v2/sandbox/d([^aad]|$);/home/userland/v1/v2/sandbox/l([^is]|$);/home/userland/v1/v2/sandbox/m([^e]|$);/home/userland/v1/v2/sandbox/o([^t]|$);/home/userland/v1/v2/sandbox/s([^crt]|$);/home/userland/v1/v2/sandbox/u([^b]|$);/home/userland/v1/v2/sandbox/v([^a]|$);/home/userland/v1/v2/sandbox/CM([^a]|$);/home/userland/v1/v2/sandbox/RE([^A]|$);/home/userland/v1/v2/sandbox/cm([^a]|$);/home/userland/v1/v2/sandbox/da([^tt]|$);/home/userland/v1/v2/sandbox/dd([^n]|$);/home/userland/v1/v2/sandbox/li([^c]|$);/home/userland/v1/v2/sandbox/ls([^a]|$);/home/userland/v1/v2/sandbox/me([^m]|$);/home/userland/v1/v2/sandbox/ot([^h]|$);/home/userland/v1/v2/sandbox/sc([^r]|$);/home/userland/v1/v2/sandbox/sr([^c]|$);/home/userland/v1/v2/sandbox/st([^o]|$);/home/userland/v1/v2/sandbox/ub([^s]|$);/home/userland/v1/v2/sandbox/va([^l]|$);/home/userland/v1/v2/sandbox/CMa([^k]|$);/home/userland/v1/v2/sandbox/REA([^D]|$);/home/userland/v1/v2/sandbox/cma([^k]|$);/home/userland/v1/v2/sandbox/dat([^aa]|$);/home/userland/v1/v2/sandbox/ddn([^e]|$);/home/userland/v1/v2/sandbox/lic([^e]|$);/home/userland/v1/v2/sandbox/lsa([^n]|$);/home/userland/v1/v2/sandbox/mem([^c]|$);/home/userland/v1/v2/sandbox/oth([^e]|$);/home/userland/v1/v2/sandbox/scr([^i]|$);/home/userland/v1/v2/sandbox/src([^/]|$);/home/userland/v1/v2/sandbox/sto([^r]|$);/home/userland/v1/v2/sandbox/ubs([^a]|$);/home/userland/v1/v2/sandbox/val([^g]|$);/home/userland/v1/v2/sandbox/CMak([^e]|$);/home/userland/v1/v2/sandbox/READ([^M]|$);/home/userland/v1/v2/sandbox/cmak([^e]|$);/home/userland/v1/v2/sandbox/data([^/s]|$);/home/userland/v1/v2/sandbox/ddne([^t]|$);/home/userland/v1/v2/sandbox/lice([^n]|$);/home/userland/v1/v2/sandbox/lsan([^.]|$);/home/userland/v1/v2/sandbox/memc([^h]|$);/home/userland/v1/v2/sandbox/othe([^r]|$);/home/userland/v1/v2/sandbox/scri([^p]|$);/home/userland/v1/v2/sandbox/stor([^a]|$);/home/userland/v1/v2/sandbox/ubsa([^n]|$);/home/userland/v1/v2/sandbox/valg([^r]|$);/home/userland/v1/v2/sandbox/CMake([^L]|$);/home/userland/v1/v2/sandbox/READM([^E]|$);/home/userland/v1/v2/sandbox/cmake([^/]|$);/home/userland/v1/v2/sandbox/datas([^r]|$);/home/userland/v1/v2/sandbox/ddnet([^-]|$);/home/userland/v1/v2/sandbox/licen([^s]|$);/home/userland/v1/v2/sandbox/lsan\\.([^s]|$);/home/userland/v1/v2/sandbox/memch([^e]|$);/home/userland/v1/v2/sandbox/other([^/]|$);/home/userland/v1/v2/sandbox/scrip([^t]|$);/home/userland/v1/v2/sandbox/stora([^g]|$);/home/userland/v1/v2/sandbox/ubsan([^.]|$);/home/userland/v1/v2/sandbox/valgr([^i]|$);/home/userland/v1/v2/sandbox/CMakeL([^i]|$);/home/userland/v1/v2/sandbox/README([^.]|$);/home/userland/v1/v2/sandbox/datasr([^c]|$);/home/userland/v1/v2/sandbox/ddnet-([^l]|$);/home/userland/v1/v2/sandbox/licens([^e]|$);/home/userland/v1/v2/sandbox/lsan\\.s([^u]|$);/home/userland/v1/v2/sandbox/memche([^c]|$);/home/userland/v1/v2/sandbox/script([^s]|$);/home/userland/v1/v2/sandbox/storag([^e]|$);/home/userland/v1/v2/sandbox/ubsan\\.([^s]|$);/home/userland/v1/v2/sandbox/valgri([^n]|$);/home/userland/v1/v2/sandbox/CMakeLi([^s]|$);/home/userland/v1/v2/sandbox/README\\.([^m]|$);/home/userland/v1/v2/sandbox/datasrc([^/]|$);/home/userland/v1/v2/sandbox/ddnet-l([^i]|$);/home/userland/v1/v2/sandbox/license([^.]|$);/home/userland/v1/v2/sandbox/lsan\\.su([^p]|$);/home/userland/v1/v2/sandbox/memchec([^k]|$);/home/userland/v1/v2/sandbox/scripts([^/]|$);/home/userland/v1/v2/sandbox/storage([^.]|$);/home/userland/v1/v2/sandbox/ubsan\\.s([^u]|$);/home/userland/v1/v2/sandbox/valgrin([^d]|$);/home/userland/v1/v2/sandbox/CMakeLis([^t]|$);/home/userland/v1/v2/sandbox/README\\.m([^d]|$);/home/userland/v1/v2/sandbox/ddnet-li([^b]|$);/home/userland/v1/v2/sandbox/license\\.([^t]|$);/home/userland/v1/v2/sandbox/lsan\\.sup([^p]|$);/home/userland/v1/v2/sandbox/memcheck([^.]|$);/home/userland/v1/v2/sandbox/storage\\.([^c]|$);/home/userland/v1/v2/sandbox/ubsan\\.su([^p]|$);/home/userland/v1/v2/sandbox/valgrind([^.]|$);/home/userland/v1/v2/sandbox/CMakeList([^s]|$);/home/userland/v1/v2/sandbox/ddnet-lib([^s]|$);/home/userland/v1/v2/sandbox/license\\.t([^x]|$);/home/userland/v1/v2/sandbox/memcheck\\.([^s]|$);/home/userland/v1/v2/sandbox/storage\\.c([^f]|$);/home/userland/v1/v2/sandbox/ubsan\\.sup([^p]|$);/home/userland/v1/v2/sandbox/valgrind\\.([^s]|$);/home/userland/v1/v2/sandbox/CMakeLists([^.]|$);/home/userland/v1/v2/sandbox/ddnet-libs([^/]|$);/home/userland/v1/v2/sandbox/license\\.tx([^t]|$);/home/userland/v1/v2/sandbox/memcheck\\.s([^u]|$);/home/userland/v1/v2/sandbox/storage\\.cf([^g]|$);/home/userland/v1/v2/sandbox/valgrind\\.s([^u]|$);/home/userland/v1/v2/sandbox/CMakeLists\\.([^t]|$);/home/userland/v1/v2/sandbox/memcheck\\.su([^p]|$);/home/userland/v1/v2/sandbox/valgrind\\.su([^p]|$);/home/userland/v1/v2/sandbox/CMakeLists\\.t([^x]|$);/home/userland/v1/v2/sandbox/memcheck\\.sup([^p]|$);/home/userland/v1/v2/sandbox/valgrind\\.sup([^p]|$);/home/userland/v1/v2/sandbox/CMakeLists\\.tx([^t]|$)")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/home/userland/v1/v2/sandbox/CPackSourceConfig.cmake")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "DDNet-19.3-src")
set(CPACK_STRIP_FILES "TRUE")
set(CPACK_SYSTEM_NAME "linux_aarch64")
set(CPACK_TOPLEVEL_TAG "linux_aarch64")
set(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/home/userland/v1/v2/sandbox/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
