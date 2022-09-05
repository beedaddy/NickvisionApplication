#include "appinfo.hpp"

using namespace NickvisionApplication::Models;

AppInfo::AppInfo() : m_name{ "" }, m_shortName{ "" }, m_description{ "" }, m_version{ "" }, m_changelog{ "" }, m_gitHubRepo{ "" }, m_issueTracker{ "" }
{

}

const std::string& AppInfo::getName() const
{
	return m_name;
}

void AppInfo::setName(const std::string& name)
{
	m_name = name;
}

const std::string& AppInfo::getShortName() const
{
	return m_shortName;
}

void AppInfo::setShortName(const std::string& shortName)
{
	m_shortName = shortName;
}

const std::string& AppInfo::getDescription() const
{
	return m_description;
}

void AppInfo::setDescription(const std::string& description)
{
	m_description = description;
}

const std::string& AppInfo::getVersion() const
{
	return m_version;
}

void AppInfo::setVersion(const std::string& version)
{
	m_version = version;
}

const std::string& AppInfo::getChangelog() const
{
	return m_changelog;
}

void AppInfo::setChangelog(const std::string& changelog)
{
	m_changelog = changelog;
}

const std::string& AppInfo::getGitHubRepo() const
{
	return m_gitHubRepo;
}

void AppInfo::setGitHubRepo(const std::string& gitHubRepo)
{
	m_gitHubRepo = gitHubRepo;
}

const std::string& AppInfo::getIssueTracker() const
{
	return m_issueTracker;
}

void AppInfo::setIssueTracker(const std::string& issueTracker)
{
	m_issueTracker = issueTracker;
}