set(src_dirs
            IIC
            LCD
            LED
            SPI
            XL9555
            FONT
            PICTURE
            CAMERA)

set(include_dirs
            IIC
            LCD
            LED
            SPI
            XL9555
            FONT
            CAMERA) 

set(requires
            driver
            esp_lcd
            esp32-camera)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

component_compile_options(-ffast-math -O3 -Wno-error=format -Wno-format)