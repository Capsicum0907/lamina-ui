foreach(var INPUT OUTPUT HEADER NAMESPACE ID)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "usage: cmake -DINPUT=<file> -DOUTPUT=<source> -DHEADER=<header> -DNAMESPACE=<namespace> -DID=<name> -P lamina_embed.cmake")
    endif()
endforeach()

file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" digits)
math(EXPR size "${digits} / 2")
if(size EQUAL 0)
    set(body "0x00,")
else()
    string(REPEAT "[0-9a-f][0-9a-f]" 32 row)
    string(REGEX REPLACE "(${row})" "\\1\n" hex "${hex}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," body "${hex}")
endif()

file(WRITE "${OUTPUT}" "#include \"${HEADER}\"

namespace {

const unsigned char data[] = {
${body}
};

}

namespace ${NAMESPACE} {

constinit const ::lamina::resources::Resource ${ID}{\"${ID}\", {data, ${size}}};

}
")
