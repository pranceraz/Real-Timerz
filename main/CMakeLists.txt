idf_component_register(SRCS "espnow_example.c" "input_handler.c" "metronome.c" "main.c" "songs.c" 
                    PRIV_REQUIRES spi_flash
                    INCLUDE_DIRS "."                  
                    REQUIRES nvs_flash esp_event esp_wifi driver)
