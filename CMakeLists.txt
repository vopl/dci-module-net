include(dciIntegrationUnit)
dciIntegrationUnit(module-net
        IDL_DIRS idl
        DEPENDS host)

file(GLOB_RECURSE SRC src/*)
file(GLOB_RECURSE IDL idl/*)

add_library(${UNAME} MODULE ${INC} ${SRC} ${IDL})
target_include_directories(${UNAME} PRIVATE src)

target_link_libraries(${UNAME} PRIVATE
    anl
    pthread)

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
        host
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
