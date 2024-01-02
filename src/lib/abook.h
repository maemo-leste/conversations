#ifndef CONV_ABOOK_H
#define CONV_ABOOK_H
#undef signals
/*Work around issues with signals being defined earlier */
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libosso-abook/osso-abook.h>

static bool conv_abook_inited = FALSE;

static OssoABookRoster *conv_abook_roster = NULL;
static OssoABookAggregator *conv_abook_aggregator = NULL;

bool conv_abook_init();

/* Look up user data given a phone number */
char* conv_abook_lookup_tel(const char* telno);
char* conv_abook_lookup_sip(const char* sipno);
char* conv_abook_lookup_im(const char* userid);

#endif /* CONV_ABOOK_H */
