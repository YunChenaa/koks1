#pragma once

enum Partition { PARTITION_CONFIG, PARTITION_BK };

void config_bk_init();
void config_bk_check_file(const char *filename);
int config_bk_update(const char* filename, bool force);
int config_bk_remount(int partition, bool readonly);
