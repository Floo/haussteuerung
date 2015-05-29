#iniparser source files have to be renamed in *.cpp to force compile using g++

set(libIniparserSrcs iniparser.cpp dictionary.cpp strlib.cpp)

add_library(iniparser ${libIniparserSrcs})

set_target_properties(iniparser PROPERTIES HAS_CXX yes)

install(TARGETS iniparser DESTINATION lib)
