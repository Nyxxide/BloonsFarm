file(WRITE "${OUTPUT_QRC}" "<RCC>\n  <qresource prefix=\"/resources\">\n")

file(GLOB_RECURSE RES_FILES
        RELATIVE "${RESOURCE_DIR}"
        "${RESOURCE_DIR}/*.png"
        "${RESOURCE_DIR}/*.jpg"
        "${RESOURCE_DIR}/*.jpeg"
        "${RESOURCE_DIR}/*.ico"
        "${RESOURCE_DIR}/*.icn"
        "${RESOURCE_DIR}/*.ttf"
        "${RESOURCE_DIR}/*.wav"
        "${RESOURCE_DIR}/*.json"
        "${RESOURCE_DIR}/*.bmp")

foreach(f IN LISTS RES_FILES)
    # absolute-or-parent-relative path on disk, clean alias in memory
    file(APPEND "${OUTPUT_QRC}"
            "    <file alias=\"${f}\">${RESOURCE_DIR}/${f}</file>\n")
endforeach()

file(APPEND "${OUTPUT_QRC}" "  </qresource>\n</RCC>\n")
