idf_component_register(
    SRCS "SDCard.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES fatfs sd_card Common
)
