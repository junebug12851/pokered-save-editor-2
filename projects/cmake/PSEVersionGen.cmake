# PSEVersionGen.cmake -- run at BUILD time via `cmake -P` (see top-level
# CMakeLists' pse_version_gen target). Re-derives the git commit metadata on
# every build and regenerates the version header from the template. configure_file
# only rewrites the header when its contents actually change, so a no-op rebuild
# does not force a recompile.
#
# Expects these to be passed with -D:
#   PSE_MODULE_DIR     dir containing PSEVersion.cmake
#   PSE_SRC_DIR        a path inside the git work tree (for the commit lookup)
#   PSE_VERSION_MAJOR  PSE_VERSION_MINOR  PSE_VERSION_PATCH
#   PSE_VERSION_HUMAN  "M.m.p[-prerelease]"
#   PSE_HEADER_IN      path to pse_version.h.in
#   PSE_HEADER_OUT     path to the generated pse_version.h

include("${PSE_MODULE_DIR}/PSEVersion.cmake")

pse_git_metadata("${PSE_SRC_DIR}" PSE_GIT_HASH PSE_GIT_DIRTY)
pse_compose_full("${PSE_VERSION_HUMAN}" "${PSE_GIT_HASH}" "${PSE_GIT_DIRTY}" PSE_VERSION_FULL)

if(PSE_GIT_DIRTY STREQUAL "")
  set(PSE_GIT_DIRTY_BOOL 0)
else()
  set(PSE_GIT_DIRTY_BOOL 1)
endif()

configure_file("${PSE_HEADER_IN}" "${PSE_HEADER_OUT}" @ONLY)
