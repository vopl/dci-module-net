include(dciIntegrationUnit)
dciIntegrationUnit(module-net
        IDL_DIRS idl
        DEPENDS host)

file(GLOB_RECURSE SRC src/*)
file(GLOB_RECURSE IDL idl/*)

if(WIN32)
    list(FILTER SRC EXCLUDE REGEX ".*enumerator.rtnetlink.(hpp|cpp)$")
else()
    list(FILTER SRC EXCLUDE REGEX ".*enumerator.netioapi.(hpp|cpp)$")
endif()

add_library(${UNAME} MODULE ${INC} ${SRC} ${IDL})
target_include_directories(${UNAME} PRIVATE src)

if(WIN32)
    target_link_libraries(${UNAME} PRIVATE Ws2_32.lib Iphlpapi.lib)
    #mingw error workaround
    #source/module/net/CMakeFiles/module-net.dir/src/entry.cpp.obj:entry.cpp:(.debug_info+0x16): relocation truncated to fit: IMAGE_REL_AMD64_SECREL against `.debug_line'
    target_link_options(${UNAME} PRIVATE -Wl,--default-image-base-low)
else()
    target_link_libraries(${UNAME} PRIVATE anl pthread)
endif()


##############################################################
include(dciIdl)
dciIdl(${UNAME} cpp
    INCLUDE ${DCI_IDL_DIRS}
    SOURCES net.idl
)

dciIdl(${UNAME} cpp
    INCLUDE ${DCI_IDL_DIRS}
    SOURCES net.idl
    OPTIONS --cpp-no-entities --cpp-stiac-support --cpp-stiac-support-filter "^dci::idl::gen::net::"
    NAME net-stiac-support
)

##############################################################
include(dciUtilsPch)
dciUtilsPch(${UNAME} src/pch.hpp ${SRC})

############################################################
include(dciHostModule)
dciHostModule(${UNAME})

##############################################################
file(GLOB_RECURSE TST test/*)
include(dciTest)
dciTest(${UNAME} mstart
    INCLUDE
        src
    SRC
        ${TST}
    LINK
        host-lib
        bytes
        utils
        sbs
        exception
        mm
        poll
        cmt
        idl
    DEPENDS
        ${UNAME}
)

if(TARGET ${UNAME}-test-mstart)
    dciIdl(${UNAME}-test-mstart cpp
        INCLUDE ${DCI_IDL_DIRS}
        SOURCES net.idl
    )
endif()
