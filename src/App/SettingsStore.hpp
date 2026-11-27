#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include "../HAL/StorageInterface.hpp"
#include "../DataModel/DataModel.hpp"

class SettingsStore {
public:
    void begin(IStorage* storage, DataModel* model);

    void saveBiometrics();
    void loadBiometrics();
    void saveBikeStats();
    void loadBikeStats();
    void saveLayout();
    void loadLayout();
    void saveTime();
    void loadTime();
    bool saveGpsSettings(const GpsSettings& settings);
    void loadGpsSettings();

private:
    IStorage* _storage = nullptr;
    DataModel* _model = nullptr;

    void replaceFile(const char* path);
};

#endif /* SETTINGS_STORE_H */
