#include "storage.h"
nvs_handle_t nvsHandle;

void initNVS(){
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ret = nvs_open("storage", NVS_READWRITE, &nvsHandle);
    if (ret != ESP_OK) {
        // Handle error
    }
}

void saveSettings() {
    printf("Saving settings\n");
    size_t blob_size = sizeof(fixture_config_t);
    esp_err_t ret = nvs_set_blob(nvsHandle, "fixture_config", &fixture_config, blob_size);
    if (ret != ESP_OK) {
        printf("NVS set_blob failed: %i\n", ret);
    }

    ret = nvs_commit(nvsHandle);
    if (ret != ESP_OK) {
        printf("NVS Commit failed: %i\n", ret);
    }
}

void loadSettings() {
    printf("Loading settings\n");
    size_t required_size;
    esp_err_t ret = nvs_get_blob(nvsHandle, "fixture_config", NULL, &required_size);

    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        printf("NVS get_blob failed: %i\n", ret);
        setFixtureDefaults();
        return;
    }

    if (required_size == sizeof(fixture_config_t)) {

        ret = nvs_get_blob(nvsHandle, "fixture_config", &fixture_config, &required_size);
        if (ret != ESP_OK) {
            printf("nvs_get_blob failed: %i\n", ret);
            setFixtureDefaults();
        }
    } else {
        printf("Wrong blob size: %zu\n", required_size);
        setFixtureDefaults();
    }
}
