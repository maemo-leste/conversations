#include "lib/abook.h"


bool conv_abook_init() {
    GError *err = NULL;
    if (!conv_abook_inited) {
        osso_abook_init_with_name("conversations", NULL);

        conv_abook_roster = osso_abook_aggregator_get_default(&err);
        if (err != NULL) {
            fprintf(stderr, "Failed to get a default aggregator\n");
            g_error_free(err);
            return FALSE;
        }
        conv_abook_aggregator = OSSO_ABOOK_AGGREGATOR(conv_abook_roster);
        printf("conv_abook_aggregator: %lx\n", conv_abook_aggregator);

        osso_abook_waitable_run(OSSO_ABOOK_WAITABLE(conv_abook_aggregator),
                    g_main_context_default(), &err);
        if (err != NULL) {
            fprintf(stderr, "Failed to wait for the aggregator\n");
            /* Free roster (not aggregator I think) */
            g_error_free(err);
            return FALSE;
        }

        conv_abook_inited = TRUE;
    }

    return TRUE;
}


OssoABookContact* conv_abook_lookup_tel(const char* telno) {
    OssoABookContact* res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_phone_number(conv_abook_aggregator, telno, TRUE);

    GList *v = l;
    while (v) {
        OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
        res = contact;
        break;
    }

    g_list_free(l);

    return res;
}


OssoABookContact* conv_abook_lookup_sip(const char* address) {
    OssoABookContact* res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_sip_address(conv_abook_aggregator, address);

    GList *v = l;
    while (v) {
        OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
        res = contact;
        break;
    }

    g_list_free(l);

    return res;
}

OssoABookContact* conv_abook_lookup_im(const char* userid) {
    OssoABookContact* res = NULL;
    GList *l = NULL;
    l = osso_abook_aggregator_find_contacts_for_im_contact(conv_abook_aggregator, userid, NULL); /* TODO: provide TpAccount* */

    GList *v = l;
    while (v) {
        OssoABookContact *contact = OSSO_ABOOK_CONTACT(v->data);
        res = contact;
        break;
    }

    g_list_free(l);

    return res;
}
