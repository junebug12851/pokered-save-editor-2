# PSEVersion.cmake -- single-source-of-truth version handling.
#
# The repo-root VERSION file is the ONLY place the human edits the version.
# This module parses it, exposes the pieces to the build, and (together with
# PSEVersionGen.cmake + pse_version.h.in) generates a C++ header the app uses
# at runtime. The git commit is appended as SemVer build metadata so dev/CI
# builds are traceable to an exact commit while the human-facing number stays a
# clean SemVer string. See notes/reference/versioning.md.
#
# Public functions:
#   pse_read_version(<path-to-VERSION>)   -> sets PSE_VERSION_* in the caller
#   pse_git_metadata(<dir> <hashVar> <dirtyVar>)
#   pse_compose_full(<human> <hash> <dirty> <outVar>)

# --- parse the VERSION file ------------------------------------------------
# Sets in PARENT_SCOPE: PSE_VERSION_MAJOR / _MINOR / _PATCH / _PRERELEASE,
# PSE_VERSION_CORE ("M.m.p", what project(VERSION) accepts) and
# PSE_VERSION_HUMAN ("M.m.p[-prerelease]", the people-facing string).
function(pse_read_version VERSION_FILE)
  if(NOT EXISTS "${VERSION_FILE}")
    message(FATAL_ERROR "PSEVersion: VERSION file not found at ${VERSION_FILE}")
  endif()

  file(STRINGS "${VERSION_FILE}" _lines ENCODING UTF-8)
  set(_ver "")
  foreach(_l IN LISTS _lines)
    string(STRIP "${_l}" _l)
    if(_l STREQUAL "" OR _l MATCHES "^#")
      continue()
    endif()
    set(_ver "${_l}")
    break()
  endforeach()

  if(_ver STREQUAL "")
    message(FATAL_ERROR "PSEVersion: no version line found in ${VERSION_FILE}")
  endif()

  # SemVer 2.0.0: MAJOR.MINOR.PATCH[-prerelease][+build]. We accept (and ignore)
  # any +build a human might type; build metadata is computed, not authored.
  if(NOT _ver MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([0-9A-Za-z.-]+))?(\\+([0-9A-Za-z.-]+))?$")
    message(FATAL_ERROR
      "PSEVersion: '${_ver}' in ${VERSION_FILE} is not valid SemVer "
      "(expected MAJOR.MINOR.PATCH[-prerelease]).")
  endif()

  set(_major "${CMAKE_MATCH_1}")
  set(_minor "${CMAKE_MATCH_2}")
  set(_patch "${CMAKE_MATCH_3}")
  set(_pre   "${CMAKE_MATCH_5}")

  set(_human "${_major}.${_minor}.${_patch}")
  if(NOT _pre STREQUAL "")
    set(_human "${_human}-${_pre}")
  endif()

  set(PSE_VERSION_MAJOR      "${_major}"                       PARENT_SCOPE)
  set(PSE_VERSION_MINOR      "${_minor}"                       PARENT_SCOPE)
  set(PSE_VERSION_PATCH      "${_patch}"                       PARENT_SCOPE)
  set(PSE_VERSION_PRERELEASE "${_pre}"                         PARENT_SCOPE)
  set(PSE_VERSION_CORE       "${_major}.${_minor}.${_patch}"   PARENT_SCOPE)
  set(PSE_VERSION_HUMAN      "${_human}"                       PARENT_SCOPE)
endfunction()

# --- capture git metadata --------------------------------------------------
# Sets <hashVar> to the short commit hash (empty if git/.git unavailable) and
# <dirtyVar> to "1" when tracked files have uncommitted changes, else "".
function(pse_git_metadata SRC_DIR OUT_HASH OUT_DIRTY)
  set(_hash "")
  set(_dirty "")

  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" -C "${SRC_DIR}" rev-parse --short=8 HEAD
      OUTPUT_VARIABLE _hash OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET RESULT_VARIABLE _rc)
    if(NOT _rc EQUAL 0)
      set(_hash "")
    endif()

    if(NOT _hash STREQUAL "")
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SRC_DIR}" status --porcelain --untracked-files=no
        OUTPUT_VARIABLE _wt OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
      if(NOT _wt STREQUAL "")
        set(_dirty "1")
      endif()
    endif()
  endif()

  set(${OUT_HASH}  "${_hash}"  PARENT_SCOPE)
  set(${OUT_DIRTY} "${_dirty}" PARENT_SCOPE)
endfunction()

# --- compose the full version string --------------------------------------
# human + "+g<hash>[.dirty]" when a hash is present, else just human.
function(pse_compose_full HUMAN HASH DIRTY OUT_FULL)
  set(_full "${HUMAN}")
  if(NOT HASH STREQUAL "")
    set(_full "${_full}+g${HASH}")
    if(NOT DIRTY STREQUAL "")
      set(_full "${_full}.dirty")
    endif()
  endif()
  set(${OUT_FULL} "${_full}" PARENT_SCOPE)
endfunction()
