# Copy only runtime assets: no Blender projects, FBX duplicates or build tools.
if(NOT DEFINED SOURCE OR NOT DEFINED EXECUTABLE OR NOT DEFINED DESTINATION)
    message(FATAL_ERROR "SOURCE, EXECUTABLE and DESTINATION required")
endif()
file(MAKE_DIRECTORY "${DESTINATION}")
file(COPY "${EXECUTABLE}" DESTINATION "${DESTINATION}")
get_filename_component(runtime_dir "${EXECUTABLE}" DIRECTORY)
foreach(runtime libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
    if(EXISTS "${runtime_dir}/${runtime}")
        file(COPY "${runtime_dir}/${runtime}" DESTINATION "${DESTINATION}")
    endif()
endforeach()
file(COPY "${SOURCE}/shaders" DESTINATION "${DESTINATION}")
file(COPY "${SOURCE}/assets/levels" DESTINATION "${DESTINATION}/assets")
file(COPY "${SOURCE}/assets/audio" DESTINATION "${DESTINATION}/assets")
foreach(model models/mirrors/mirror_complete.glb models/characters/ranger/Ranger.glb
    models/environment/doors/temple_sliding_door.glb models/traps/spike_trap.glb
    models/ruby/ruby_pedestal_floating.glb
    animations/characters/kaykit_medium/Rig_Medium_General.glb
    animations/characters/kaykit_medium/Rig_Medium_MovementBasic.glb)
    get_filename_component(directory "${model}" DIRECTORY)
    file(COPY "${SOURCE}/assets/${model}" DESTINATION "${DESTINATION}/assets/${directory}")
endforeach()
set(kit "assets/vendor/kaykit_dungeon/KayKit_Dungeon_Pack_1.1_FREE/Assets/gltf")
foreach(asset floor_tile_large.gltf floor_tile_large.bin wall.gltf wall.bin
    pillar_decorated.gltf pillar_decorated.bin dungeon_texture.png)
    file(COPY "${SOURCE}/${kit}/${asset}" DESTINATION "${DESTINATION}/${kit}")
endforeach()
file(COPY "${SOURCE}/docs/PLAYING.md" "${SOURCE}/docs/THIRD_PARTY_AUDIO.md" DESTINATION "${DESTINATION}")
file(WRITE "${DESTINATION}/Play.cmd" "@echo off\r\ncd /d \"%~dp0\"\r\nreflections_of_the_tomb.exe\r\n")
message(STATUS "Presentation build ready: ${DESTINATION}")
