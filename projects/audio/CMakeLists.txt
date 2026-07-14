# The Game Boy's sound, in software.
#
#   GbApu            -- the DMG sound chip (notes/reference/gameboy-apu.md)
#   Gen1SoundEngine  -- the game's sequencer, transliterated from pret/pokered
#                       (notes/reference/gen1-sound-engine.md)
#   Gen1MusicAsm     -- the game's own SHEET MUSIC (.asm), read as text. A LINE PARSER, not an
#                       assembler: their macro names ARE the command names. (notes/plans/music.md §8)
#
# It deliberately knows nothing about save files or QML: the engine emits APU register writes, the
# APU emits PCM, and everything above sits in `app`. See notes/plans/music.md.

qt_add_library(audio SHARED
    src/pse-audio/gbapu.cpp
    src/pse-audio/gbapu.h
    src/pse-audio/gen1soundengine.cpp
    src/pse-audio/gen1soundengine.h
    src/pse-audio/gen1musicasm.cpp
    src/pse-audio/gen1musicasm.h
    src/pse-audio/audio_autoport.h
)

target_compile_definitions(audio PRIVATE AUDIO_LIBRARY)

target_include_directories(audio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(audio
    PUBLIC
        Qt6::Core
)
