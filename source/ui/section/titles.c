#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "section.h"
#include "action/action.h"
#include "task/task.h"
#include "../error.h"
#include "../list.h"
#include "../ui.h"
#include "../../core/linkedlist.h"
#include "../../core/screen.h"
#include "../../core/util.h"

static list_item launch_title = {"Starte Title", COLOR_TEXT, action_launch_title};
static list_item delete_title = {"Lösche Title", COLOR_TEXT, action_delete_title};
static list_item delete_title_ticket = {"Lösche Title und Ticket", COLOR_TEXT, action_delete_title_ticket};
static list_item extract_smdh = {"Extrahiere SMDH", COLOR_TEXT, action_extract_smdh};
static list_item import_seed = {"Importiere Seed", COLOR_TEXT, action_import_seed};
static list_item browse_save_data = {"Suche Save Data", COLOR_TEXT, action_browse_title_save_data};
static list_item import_save_data = {"Importiere Save Data", COLOR_TEXT, action_import_twl_save};
static list_item export_save_data = {"Exportiere Save Data", COLOR_TEXT, action_export_twl_save};
static list_item erase_save_data = {"Lösche Save Data", COLOR_TEXT, action_erase_twl_save};
static list_item import_secure_value = {"Importiere Secure Value", COLOR_TEXT, action_import_secure_value};
static list_item export_secure_value = {"Exportiere Secure Value", COLOR_TEXT, action_export_secure_value};
static list_item delete_secure_value = {"Lösche Secure Value", COLOR_TEXT, action_delete_secure_value};

typedef struct {
    populate_titles_data populateData;

    bool showGameCard;
    bool showSD;
    bool showNAND;
    bool sortByName;

    bool populated;
} titles_data;

typedef struct {
    linked_list* items;
    list_item* selected;
} titles_action_data;

static void titles_action_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    ui_draw_title_info(view, ((titles_action_data*) data)->selected->data, x1, y1, x2, y2);
}

static void titles_action_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    titles_action_data* actionData = (titles_action_data*) data;

    if(hidKeysDown() & KEY_B) {
        ui_pop();
        list_destroy(view);

        free(data);

        return;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        void(*action)(linked_list*, list_item*) = (void(*)(linked_list*, list_item*)) selected->data;

        ui_pop();
        list_destroy(view);

        action(actionData->items, actionData->selected);

        free(data);

        return;
    }

    if(linked_list_size(items) == 0) {
        linked_list_add(items, &launch_title);

        title_info* info = (title_info*) actionData->selected->data;

        if(info->mediaType != MEDIATYPE_GAME_CARD) {
            linked_list_add(items, &delete_title);
            linked_list_add(items, &delete_title_ticket);
        }

        if(!info->twl) {
            linked_list_add(items, &extract_smdh);

            if(info->mediaType != MEDIATYPE_GAME_CARD) {
                linked_list_add(items, &import_seed);
            }

            linked_list_add(items, &browse_save_data);

            if(info->mediaType != MEDIATYPE_GAME_CARD) {
                linked_list_add(items, &import_secure_value);
                linked_list_add(items, &export_secure_value);
                linked_list_add(items, &delete_secure_value);
            }
        } else if(info->mediaType == MEDIATYPE_GAME_CARD) {
            linked_list_add(items, &import_save_data);
            linked_list_add(items, &export_save_data);
            linked_list_add(items, &erase_save_data);
        }
    }
}

static void titles_action_open(linked_list* items, list_item* selected) {
    titles_action_data* data = (titles_action_data*) calloc(1, sizeof(titles_action_data));
    if(data == NULL) {
        error_display(NULL, NULL, "Failed to allocate titles action data.");

        return;
    }

    data->items = items;
    data->selected = selected;

    list_display("Title Action", "A: Auswählen, B: Zurück", data, titles_action_update, titles_action_draw_top);
}

static void titles_options_add_entry(linked_list* items, const char* name, bool* val) {
    list_item* item = (list_item*) calloc(1, sizeof(list_item));
    if(item != NULL) {
        snprintf(item->name, LIST_ITEM_NAME_MAX, "%s", name);
        item->color = *val ? COLOR_ENABLED : COLOR_DISABLED;
        item->data = val;

        linked_list_add(items, item);
    }
}

static void titles_options_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    titles_data* listData = (titles_data*) data;

    if(hidKeysDown() & KEY_B) {
        linked_list_iter iter;
        linked_list_iterate(items, &iter);

        while(linked_list_iter_has_next(&iter)) {
            free(linked_list_iter_next(&iter));
            linked_list_iter_remove(&iter);
        }

        ui_pop();
        list_destroy(view);

        return;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        bool* val = (bool*) selected->data;
        *val = !(*val);

        selected->color = *val ? COLOR_ENABLED : COLOR_DISABLED;

        listData->populated = false;
    }

    if(linked_list_size(items) == 0) {
        titles_options_add_entry(items, "Zeige Game Card", &listData->showGameCard);
        titles_options_add_entry(items, "Zeige SD", &listData->showSD);
        titles_options_add_entry(items, "Zeige NAND", &listData->showNAND);
        titles_options_add_entry(items, "Sortieren nach Name", &listData->sortByName);
    }
}

static void titles_options_open(titles_data* data) {
    list_display("Optionen", "A: Toggle, B: Zurück", data, titles_options_update, NULL);
}

static void titles_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    if(selected != NULL && selected->data != NULL) {
        ui_draw_title_info(view, selected->data, x1, y1, x2, y2);
    }
}

static void titles_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    titles_data* listData = (titles_data*) data;

    if(hidKeysDown() & KEY_B) {
        if(!listData->populateData.finished) {
            svcSignalEvent(listData->populateData.cancelEvent);
            while(!listData->populateData.finished) {
                svcSleepThread(1000000);
            }
        }

        ui_pop();

        task_clear_titles(items);
        list_destroy(view);

        free(listData);
        return;
    }

    if(hidKeysDown() & KEY_SELECT) {
        titles_options_open(listData);
        return;
    }

    if(!listData->populated || (hidKeysDown() & KEY_X)) {
        if(!listData->populateData.finished) {
            svcSignalEvent(listData->populateData.cancelEvent);
            while(!listData->populateData.finished) {
                svcSleepThread(1000000);
            }
        }

        listData->populateData.items = items;
        Result res = task_populate_titles(&listData->populateData);
        if(R_FAILED(res)) {
            error_display_res(NULL, NULL, res, "Failed to initiate title list population.");
        }

        listData->populated = true;
    }

    if(listData->populateData.finished && R_FAILED(listData->populateData.result)) {
        error_display_res(NULL, NULL, listData->populateData.result, "Failed to populate title list.");

        listData->populateData.result = 0;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        titles_action_open(items, selected);
        return;
    }
}

static bool titles_filter(void* data, u64 titleId, FS_MediaType mediaType) {
    titles_data* listData = (titles_data*) data;

    if(mediaType == MEDIATYPE_GAME_CARD) {
        return listData->showGameCard;
    } else if(mediaType == MEDIATYPE_SD) {
        return listData->showSD;
    } else {
        return listData->showNAND;
    }
}

static int titles_compare(void* data, const void* p1, const void* p2) {
    titles_data* listData = (titles_data*) data;

    list_item* info1 = (list_item*) p1;
    list_item* info2 = (list_item*) p2;

    title_info* title1 = (title_info*) info1->data;
    title_info* title2 = (title_info*) info2->data;


    if(title1->mediaType > title2->mediaType) {
        return -1;
    } else if(title1->mediaType < title2->mediaType) {
        return 1;
    } else {
        if(!title1->twl && title2->twl) {
            return -1;
        } else if(title1->twl && !title2->twl) {
            return 1;
        } else {
            if(listData->sortByName) {
                bool title1HasName = title1->hasMeta && !util_is_string_empty(title1->meta.shortDescription);
                bool title2HasName = title2->hasMeta && !util_is_string_empty(title2->meta.shortDescription);

                if(title1HasName && !title2HasName) {
                    return -1;
                } else if(!title1HasName && title2HasName) {
                    return 1;
                } else {
                    return strcasecmp(info1->name, info2->name);
                }
            } else {
                u64 id1 = title1->titleId;
                u64 id2 = title2->titleId;

                return id1 > id2 ? 1 : id1 < id2 ? -1 : 0;
            }
        }
    }
}

void titles_open() {
    titles_data* data = (titles_data*) calloc(1, sizeof(titles_data));
    if(data == NULL) {
        error_display(NULL, NULL, "Failed to allocate titles data.");

        return;
    }

    data->populateData.userData = data;
    data->populateData.filter = titles_filter;
    data->populateData.compare = titles_compare;

    data->populateData.finished = true;

    data->showGameCard = true;
    data->showSD = true;
    data->showNAND = true;
    data->sortByName = true;

    list_display("Titles", "A: Auswählen, B: Zurück, X: Aktualisieren, Select: Optionen", data, titles_update, titles_draw_top);
}