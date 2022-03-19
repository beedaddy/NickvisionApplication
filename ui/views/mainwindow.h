#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <string>
#include <adwaita.h>
#include "../../update/updater.h"
#include "welcomepage.h"
#include "formpage.h"

namespace NickvisionApplication::UI::Views
{
    typedef void (*Callback_GioAction)(GSimpleAction*, GVariant*, gpointer*);
    typedef void (*Callback_GtkWidget)(GtkWidget*, gpointer*);
    typedef void (*Callback_GtkDialog_Response)(GtkDialog*, gint, gpointer*);
    typedef void (*Callback_GtkListBox_Selection)(GtkListBox*, GtkListBoxRow*, gpointer*);

    class MainWindow
    {
    public:
        MainWindow();
        ~MainWindow();
        GtkWidget* gobj();
        GtkBuilder* getBuilder();
        void show();
        void showMaximized();

    private:
        GtkBuilder* m_builder = nullptr;
        NickvisionApplication::Update::Updater m_updater;
        //==Help Actions==//
        GSimpleAction* m_gio_actCheckForUpdates = nullptr;
        GSimpleAction* m_gio_actGitHubRepo = nullptr;
        GSimpleAction* m_gio_actReportABug = nullptr;
        GSimpleAction* m_gio_actPreferences = nullptr;
        GSimpleAction* m_gio_actChangelog = nullptr;
        GSimpleAction* m_gio_actAbout = nullptr;
        //==Pages==//
        WelcomePage m_welcomePage;
        FormPage m_formPage;
        //==Signals==//
        void checkForUpdates();
        void gitHubRepo();
        void reportABug();
        void preferences();
        void changelog();
        void about();
        void onNavigationChanged();
        //==Messages==//
        void sendToast(const std::string& message);
    };
}

#endif // MAINWINDOW_H