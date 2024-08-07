#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include <format>
#include <libnick/helpers/codehelpers.h>
#include <libnick/helpers/stringhelpers.h>
#include <libnick/notifications/shellnotification.h>
#include <libnick/localization/gettext.h>
#include "SettingsPage.xaml.h"

using namespace ::Nickvision;
using namespace ::Nickvision::Application::Shared::Controllers;
using namespace ::Nickvision::Application::Shared::Models;
using namespace ::Nickvision::Events;
using namespace ::Nickvision::Helpers;
using namespace ::Nickvision::Notifications;
using namespace ::Nickvision::Update;
using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Microsoft::UI::Xaml::Controls::Primitives;
using namespace winrt::Microsoft::UI::Xaml::Input;
using namespace winrt::Microsoft::UI::Xaml::Media;
using namespace winrt::Microsoft::UI::Windowing;
using namespace winrt::Windows::ApplicationModel::DataTransfer;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Pickers;
using namespace winrt::Windows::System;

namespace winrt::Nickvision::Application::WinUI::implementation 
{
    static std::vector<std::string> keys(const std::unordered_map<std::string, std::string>& m)
    {
        std::vector<std::string> k;
        for(std::unordered_map<std::string, std::string>::const_iterator it = m.begin(); it != m.end(); it++)
        {
            k.push_back(it->first);
        }
        return k;
    }

    MainWindow::MainWindow()
        : m_opened{ false },
        m_notificationClickToken{ 0 }
    {
        InitializeComponent();
        this->m_inner.as<::IWindowNative>()->get_WindowHandle(&m_hwnd);
        //Set TitleBar
        TitleBar().AppWindow(AppWindow());
        //Localize Strings
        NavViewHome().Content(winrt::box_value(winrt::to_hstring(_("Home"))));
        NavViewFolder().Content(winrt::box_value(winrt::to_hstring(_("Folder"))));
        NavViewHelp().Content(winrt::box_value(winrt::to_hstring(_("Help"))));
        ToolTipService::SetToolTip(BtnCheckForUpdates(), winrt::box_value(winrt::to_hstring(_("Check for Updates"))));
        ToolTipService::SetToolTip(BtnCredits(), winrt::box_value(winrt::to_hstring(_("Credits"))));
        ToolTipService::SetToolTip(BtnCopyDebugInfo(), winrt::box_value(winrt::to_hstring(_("Copy Debug Information"))));
        LblChangelog().Text(winrt::to_hstring(_("Changelog")));
        BtnGitHubRepo().Content(winrt::box_value(winrt::to_hstring(_("GitHub Repo"))));
        BtnReportABug().Content(winrt::box_value(winrt::to_hstring(_("Report a Bug"))));
        BtnDiscussions().Content(winrt::box_value(winrt::to_hstring(_("Discussions"))));
        NavViewSettings().Content(winrt::box_value(winrt::to_hstring(_("Settings"))));
        StatusPageHome().Description(winrt::to_hstring(_("Open a folder (or drag one into the app) to get started")));
        LblBtnHomeOpenFolder().Text(winrt::to_hstring(_("Open Folder")));
        BtnFolderOpenFolder().Label(winrt::to_hstring(_("Open")));
        ToolTipService::SetToolTip(BtnFolderCloseFolder(), winrt::box_value(winrt::to_hstring(_("Close (Ctrl+W)"))));
        StatusPageNoFiles().Title(winrt::to_hstring(_("No files in folder")));
    }

    void MainWindow::SetController(const std::shared_ptr<MainWindowController>& controller, ElementTheme systemTheme)
    {
        m_controller = controller;
        m_systemTheme = systemTheme;
        //Register Events
        AppWindow().Closing({ this, &MainWindow::OnClosing });
        m_controller->configurationSaved() += [&](const EventArgs& args) { OnConfigurationSaved(args); };
        m_controller->notificationSent() += [&](const NotificationSentEventArgs& args) { DispatcherQueue().TryEnqueue([this, args](){ OnNotificationSent(args); }); };
        m_controller->shellNotificationSent() += [&](const ShellNotificationSentEventArgs& args) { OnShellNotificationSent(args); };
        m_controller->folderChanged() += [&](const EventArgs& args) { OnFolderChanged(args); };
        //Localize Strings
        TitleBar().Title(winrt::to_hstring(m_controller->getAppInfo().getShortName()));
        TitleBar().Subtitle(m_controller->getAppInfo().getVersion().getVersionType() == VersionType::Preview ? winrt::to_hstring(_("Preview")) : L"");
        LblAppName().Text(winrt::to_hstring(m_controller->getAppInfo().getShortName()));
        LblAppDescription().Text(winrt::to_hstring(m_controller->getAppInfo().getDescription()));
        LblAppVersion().Text(winrt::to_hstring(m_controller->getAppInfo().getVersion().str()));
        LblAppChangelog().Text(winrt::to_hstring(m_controller->getAppInfo().getChangelog()));
        StatusPageHome().Title(winrt::to_hstring(m_controller->getGreeting()));
    }

    void MainWindow::OnLoaded(const IInspectable& sender, const RoutedEventArgs& args)
    {
        if (!m_controller)
        {
            throw std::logic_error("MainWindow::SetController() must be called before using the window.");
        }
        if (m_opened)
        {
            return;
        }
        m_controller->startup(m_hwnd).apply(m_hwnd);
        NavViewHome().IsSelected(true);
        m_opened = true;
    }

    void MainWindow::OnClosing(const Microsoft::UI::Windowing::AppWindow& sender, const AppWindowClosingEventArgs& args)
    {
        if(!m_controller->canShutdown())
        {
            args.Cancel(true);
            return;
        }
        m_controller->shutdown({ m_hwnd });
    }

    void MainWindow::OnActivated(const IInspectable& sender, const WindowActivatedEventArgs& args)
    {
        if(args.WindowActivationState() != WindowActivationState::Deactivated)
        {
            switch(MainGrid().ActualTheme())
            {
            case ElementTheme::Light:
                TitleBar().TitleForeground(SolidColorBrush(Colors::Black()));
                break;
            case ElementTheme::Dark:
                TitleBar().TitleForeground(SolidColorBrush(Colors::White()));
                break;
            default:
                break;
            }
        }
        else
        {
            TitleBar().TitleForeground(SolidColorBrush(Colors::Gray()));
        }
    }

    void MainWindow::OnDragOver(const IInspectable& sender, const DragEventArgs& args)
    {
        args.AcceptedOperation(DataPackageOperation::Copy | DataPackageOperation::Link);
        args.DragUIOverride().Caption(winrt::to_hstring(_("Drop here to open folder")));
        args.DragUIOverride().IsGlyphVisible(true);
        args.DragUIOverride().IsContentVisible(true);
        args.DragUIOverride().IsCaptionVisible(true);
    }

    Windows::Foundation::IAsyncAction MainWindow::OnDrop(const IInspectable& sender, const DragEventArgs& args)
    {
        if (args.DataView().Contains(StandardDataFormats::StorageItems()))
        {
            IVectorView<IStorageItem> items{ co_await args.DataView().GetStorageItemsAsync() };
            if (items.Size() > 0)
            {
                m_controller->openFolder(winrt::to_string(items.GetAt(0).Path()));
            }
        }
    }

    void MainWindow::OnConfigurationSaved(const EventArgs& args)
    {
        switch (m_controller->getTheme())
        {
        case Theme::Light:
            MainGrid().RequestedTheme(ElementTheme::Light);
            break;
        case Theme::Dark:
            MainGrid().RequestedTheme(ElementTheme::Dark);
            break;
        default:
            MainGrid().RequestedTheme(m_systemTheme);
            break;
        }
    }

    void MainWindow::OnNotificationSent(const NotificationSentEventArgs& args)
    {
        InfoBar().Message(winrt::to_hstring(args.getMessage()));
        switch(args.getSeverity())
        {
        case NotificationSeverity::Success:
            InfoBar().Severity(InfoBarSeverity::Success);
            break;
        case NotificationSeverity::Warning:
            InfoBar().Severity(InfoBarSeverity::Warning);
            break;
        case NotificationSeverity::Error:
            InfoBar().Severity(InfoBarSeverity::Error);
            break;
        default:
            InfoBar().Severity(InfoBarSeverity::Informational);
            break;
        }
        if(m_notificationClickToken)
        {
            BtnInfoBar().Click(m_notificationClickToken);
        }
        if(args.getAction() == "error")
        {
            NavView().SelectedItem(nullptr);
            if(m_controller->isFolderOpened())
            {
                NavViewFolder().IsSelected(true);
            }
            else
            {
                NavViewHome().IsSelected(true);
            }
        }
        else if(args.getAction() == "update")
        {
            BtnInfoBar().Content(winrt::box_value(winrt::to_hstring(_("Update"))));
            m_notificationClickToken = BtnInfoBar().Click({ this, &MainWindow::WindowsUpdate });
        }
        else if(args.getAction() == "close")
        {
            BtnInfoBar().Content(winrt::box_value(winrt::to_hstring(_("Close"))));
            m_notificationClickToken = BtnInfoBar().Click({ this, &MainWindow::CloseFolder });
        }
        BtnInfoBar().Visibility(!args.getAction().empty() ? Visibility::Visible : Visibility::Collapsed);
        InfoBar().IsOpen(true);
    }

    void MainWindow::OnShellNotificationSent(const ShellNotificationSentEventArgs& args)
    {
        m_controller->log(Logging::LogLevel::Info, "ShellNotification sent. (" + args.getMessage() + ")");
        ShellNotification::send(args, m_hwnd);
    }

    void MainWindow::OnNavSelectionChanged(const NavigationView& sender, const NavigationViewSelectionChangedEventArgs& args)
    {
        winrt::hstring tag{ NavView().SelectedItem().as<NavigationViewItem>().Tag().as<winrt::hstring>() };
        if(tag == L"Settings")
        {
            WinUI::SettingsPage page{ winrt::make<SettingsPage>() };
            page.as<SettingsPage>()->SetController(m_controller->createPreferencesViewController());
            ViewStack().CurrentPage(L"Custom");
            FrameCustom().Content(winrt::box_value(page));
        }
        else
        {
            ViewStack().CurrentPage(tag);
        }
    }

    void MainWindow::OnNavViewItemTapped(const IInspectable& sender, const TappedRoutedEventArgs& args)
    {
        FlyoutBase::ShowAttachedFlyout(sender.as<FrameworkElement>());
    }

    void MainWindow::CheckForUpdates(const IInspectable& sender, const RoutedEventArgs& args)
    {
        FlyoutBase::GetAttachedFlyout(NavViewHelp().as<FrameworkElement>()).Hide();
        m_controller->checkForUpdates();
    }

    void MainWindow::WindowsUpdate(const IInspectable& sender, const RoutedEventArgs& args)
    {
        InfoBar().IsOpen(false);
        NavView().IsEnabled(false);
        ViewStack().CurrentPage(L"Spinner");
        m_controller->windowsUpdate();
    }

    void MainWindow::CopyDebugInformation(const IInspectable& sender, const RoutedEventArgs& args)
    {
        FlyoutBase::GetAttachedFlyout(NavViewHelp().as<FrameworkElement>()).Hide();
        DataPackage dataPackage;
        dataPackage.SetText(winrt::to_hstring(m_controller->getDebugInformation()));
        Clipboard::SetContent(dataPackage);
        OnNotificationSent({ _("Debug information copied to clipboard."), NotificationSeverity::Success });
    }

    Windows::Foundation::IAsyncAction MainWindow::GitHubRepo(const IInspectable& sender, const RoutedEventArgs& args)
    {
        co_await Launcher::LaunchUriAsync(Windows::Foundation::Uri{ winrt::to_hstring(m_controller->getAppInfo().getSourceRepo()) });
    }

    Windows::Foundation::IAsyncAction MainWindow::ReportABug(const IInspectable& sender, const RoutedEventArgs& args)
    {
        co_await Launcher::LaunchUriAsync(Windows::Foundation::Uri{ winrt::to_hstring(m_controller->getAppInfo().getIssueTracker()) });
    }

    Windows::Foundation::IAsyncAction MainWindow::Discussions(const IInspectable& sender, const RoutedEventArgs& args)
    {
        co_await Launcher::LaunchUriAsync(Windows::Foundation::Uri{ winrt::to_hstring(m_controller->getAppInfo().getSupportUrl()) });
    }

    Windows::Foundation::IAsyncAction MainWindow::Credits(const IInspectable& sender, const RoutedEventArgs& args)
    {
        FlyoutBase::GetAttachedFlyout(NavViewHelp().as<FrameworkElement>()).Hide();
        ContentDialog dialog;
        dialog.Title(winrt::box_value(winrt::to_hstring(_("Credits"))));
        if(m_controller->getAppInfo().getTranslatorNames().size() == 1 && m_controller->getAppInfo().getTranslatorNames()[0] == "translator-credits")
        {
            dialog.Content(winrt::box_value(winrt::to_hstring(std::vformat(_("Developers:\n{}\nDesigners:\n{}\nArtists:\n{}"), std::make_format_args(CodeHelpers::unmove(StringHelpers::join(keys(m_controller->getAppInfo().getDevelopers()), "\n")), CodeHelpers::unmove(StringHelpers::join(keys(m_controller->getAppInfo().getDesigners()), "\n")), CodeHelpers::unmove(StringHelpers::join(keys(m_controller->getAppInfo().getArtists()), "\n", false)))))));
        }
        else
        {
            dialog.Content(winrt::box_value(winrt::to_hstring(std::vformat(_("Developers:\n{}\nDesigners:\n{}\nArtists:\n{}\nTranslators:\n{}"), std::make_format_args(CodeHelpers::unmove(StringHelpers::join(keys(m_controller->getAppInfo().getDevelopers()), "\n")), CodeHelpers::unmove(StringHelpers::join(keys(m_controller->getAppInfo().getDesigners()), "\n")), CodeHelpers::unmove(StringHelpers::join(keys(m_controller->getAppInfo().getArtists()), "\n")), CodeHelpers::unmove(StringHelpers::join(m_controller->getAppInfo().getTranslatorNames(), "\n", false)))))));
        }
        dialog.CloseButtonText(winrt::to_hstring(_("Close")));
        dialog.DefaultButton(ContentDialogButton::Close);
        dialog.RequestedTheme(MainGrid().ActualTheme());
        dialog.XamlRoot(MainGrid().XamlRoot());
        co_await dialog.ShowAsync();
    }

    void MainWindow::OnFolderChanged(const EventArgs& args)
    {
        NavViewHome().IsSelected(!m_controller->isFolderOpened());
        NavViewFolder().IsEnabled(m_controller->isFolderOpened());
        NavViewFolder().IsSelected(m_controller->isFolderOpened());
        ListFiles().Items().Clear();
        if(m_controller->isFolderOpened())
        {
            if(m_controller->getFiles().empty())
            {
                ViewStackFolder().CurrentPage(L"NoFiles");
            }
            else
            {
                ViewStackFolder().CurrentPage(L"Files");
                StatusPageFiles().Description(winrt::to_hstring(std::vformat(_n("There is {} file in the folder.", "There are {} files in the folder.", m_controller->getFiles().size()), std::make_format_args(CodeHelpers::unmove(m_controller->getFiles().size())))));
                for(const std::filesystem::path& file : m_controller->getFiles())
                {
                    StackPanel fileRow;
                    fileRow.Margin({ 6, 6, 6, 6 });
                    fileRow.Orientation(Orientation::Vertical);
                    fileRow.Spacing(6);
                    TextBlock lblFilename;
                    lblFilename.Text(winrt::to_hstring(file.filename().string()));
                    TextBlock lblPath;
                    lblPath.Text(winrt::to_hstring(file.string()));
                    lblPath.Foreground(SolidColorBrush(Colors::Gray()));
                    fileRow.Children().Append(lblFilename);
                    fileRow.Children().Append(lblPath);
                    ListFiles().Items().Append(fileRow);
                }
            }
        }
    }

    Windows::Foundation::IAsyncAction MainWindow::OpenFolder(const IInspectable& sender, const RoutedEventArgs& args)
    {
        FolderPicker picker;
        picker.as<::IInitializeWithWindow>()->Initialize(m_hwnd);
        picker.FileTypeFilter().Append(L"*");
        StorageFolder folder{ co_await picker.PickSingleFolderAsync() };
        if(folder)
        {
            m_controller->openFolder(winrt::to_string(folder.Path()));
        }
    }

    void MainWindow::CloseFolder(const IInspectable& sender, const RoutedEventArgs& args)
    {
        InfoBar().IsOpen(false);
        m_controller->closeFolder();
    }
}
