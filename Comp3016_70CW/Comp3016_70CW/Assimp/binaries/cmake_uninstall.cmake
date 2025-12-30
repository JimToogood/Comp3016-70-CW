IF(NOT EXISTS "C:/Users/jimto/source/repos/test_project/test_project/Assimp/binaries/install_manifest.txt")
  MESSAGE(FATAL_ERROR "Cannot find install manifest: \"C:/Users/jimto/source/repos/test_project/test_project/Assimp/binaries/install_manifest.txt\"")
ENDIF(NOT EXISTS "C:/Users/jimto/source/repos/test_project/test_project/Assimp/binaries/install_manifest.txt")

FILE(READ "C:/Users/jimto/source/repos/test_project/test_project/Assimp/binaries/install_manifest.txt" files)
STRING(REGEX REPLACE "\n" ";" files "${files}")
FOREACH(file ${files})
  MESSAGE(STATUS "Uninstalling \"$ENV{DESTDIR}${file}\"")
  EXEC_PROGRAM(
    "C:/Users/jimto/Downloads/cmake-4.2.1-windows-x86_64/cmake-4.2.1-windows-x86_64/bin/cmake.exe" ARGS "-E remove \"$ENV{DESTDIR}${file}\""
    OUTPUT_VARIABLE rm_out
    RETURN_VALUE rm_retval
    )
  IF(NOT "${rm_retval}" STREQUAL 0)
    MESSAGE(FATAL_ERROR "Problem when removing \"$ENV{DESTDIR}${file}\"")
  ENDIF(NOT "${rm_retval}" STREQUAL 0)
ENDFOREACH(file)
