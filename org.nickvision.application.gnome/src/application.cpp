#include "application.h"
#include <stdexcept>
#include <libnick/app/aura.h>

using namespace Nickvision::App;
using namespace Nickvision::Application::Shared::Controllers;
using namespace Nickvision::Application::Shared::Models;

namespace Nickvision::Application::GNOME
{
    Application::Application(int argc, char* argv[])
        : m_controller{ std::make_shared<MainWindowController>(std::vector<std::string>(argv, argv + argc)) },
        m_adw{ adw_application_new(m_controller->getAppInfo().getId().c_str(), G_APPLICATION_DEFAULT_FLAGS) },
        m_mainWindow{ nullptr }
    {
        m_controller->getAppInfo().setChangelog("- Initial Release");
        std::filesystem::path resources{ Aura::getActive().getExecutableDirectory() / (m_controller->getAppInfo().getId() + ".gresource") };
        GError* resourceLoadError{ nullptr };
        GResource* resource{ g_resource_load(resources.string().c_str(), &resourceLoadError) };
        if(resourceLoadError)
        {
            Aura::getActive().getLogger().log(Logging::LogLevel::Critical, "Unable to load GResource. " + std::string(resourceLoadError->message));
            throw std::runtime_error(resourceLoadError->message);
        }
        g_resources_register(resource);
        g_signal_connect(m_adw, "activate", G_CALLBACK(+[](GtkApplication* app, gpointer data){ reinterpret_cast<Application*>(data)->onActivate(app); }), this);
    }

    int Application::run()
    {
        Aura::getActive().getLogger().log(Logging::LogLevel::Debug, "Started GTK application.");
        return g_application_run(G_APPLICATION(m_adw), 0, nullptr);
    }

    void Application::onActivate(GtkApplication* app)
    {
        switch (m_controller->getTheme())
        {
        case Theme::Light:
            adw_style_manager_set_color_scheme(adw_style_manager_get_default(), ADW_COLOR_SCHEME_FORCE_LIGHT);
            break;
        case Theme::Dark:
            adw_style_manager_set_color_scheme(adw_style_manager_get_default(), ADW_COLOR_SCHEME_FORCE_DARK);
            break;
        default:
            adw_style_manager_set_color_scheme(adw_style_manager_get_default(), ADW_COLOR_SCHEME_DEFAULT);
            break;
        }
        if(!m_mainWindow)
        {
            m_mainWindow = std::make_shared<Views::MainWindow>(m_controller, app);
        }
        m_mainWindow->show();
    }
}