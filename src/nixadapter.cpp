#include "nixadapter.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

using yas::CliAction;
using yas::CliCommand;
using yas::Package;

// Nix adapter targeting the new CLI (`nix profile`, `nix search`).
// Experimental features are enabled per-invocation so the app works without
// touching the user's nix.conf. Only user profiles are managed — NixOS
// system configuration is out of scope by design.
namespace {

const QString kNix = QStringLiteral("nix");

QStringList exp()
{
    return {QStringLiteral("--extra-experimental-features"),
            QStringLiteral("nix-command flakes")};
}

CliCommand nix(QStringList args)
{
    return {kNix, exp() + args};
}

// "legacyPackages.<system>.attr.path" -> "attr.path"
QString attrToId(const QString &attrKey)
{
    const QStringList parts = attrKey.split(QLatin1Char('.'));
    if (parts.size() <= 2)
        return attrKey;
    return QStringList(parts.mid(2)).join(QLatin1Char('.'));
}

// "hash-name-1.2.3" -> version "1.2.3" (best effort)
QString versionFromStorePath(const QString &storePath)
{
    static const QRegularExpression versionRe(
        QStringLiteral("-([0-9][0-9A-Za-z.+_]*)$"));
    const QString base = storePath.section(QLatin1Char('/'), -1);
    const auto match = versionRe.match(base);
    return match.hasMatch() ? match.captured(1) : QString();
}

} // namespace

QString NixAdapter::displayName() const { return QStringLiteral("Nix"); }
QString NixAdapter::cliProgram() const { return kNix; }
QStringList NixAdapter::cliSearchPaths() const
{
    return {QStringLiteral("/nix/var/nix/profiles/default/bin"),
            QDir::homePath() + QStringLiteral("/.nix-profile/bin"),
            QStringLiteral("/run/current-system/sw/bin")};
}
QStringList NixAdapter::cliVersionArguments() const { return {QStringLiteral("--version")}; }

CliCommand NixAdapter::searchCommand(const QString &query) const
{
    return nix({QStringLiteral("search"), QStringLiteral("nixpkgs"), query,
                QStringLiteral("--json")});
}

CliCommand NixAdapter::infoCommand(const QString &packageId, const QString &) const
{
    // No dedicated info command in the new CLI — exact-match search.
    return nix({QStringLiteral("search"), QStringLiteral("nixpkgs"),
                QLatin1Char('^') + packageId + QLatin1Char('$'), QStringLiteral("--json")});
}

CliCommand NixAdapter::listInstalledCommand() const
{
    return nix({QStringLiteral("profile"), QStringLiteral("list"), QStringLiteral("--json")});
}

CliCommand NixAdapter::listOutdatedCommand() const
{
    // The new CLI has no "list upgradable" — Updates view stays empty and
    // "Upgrade all" performs `nix profile upgrade --all` (TODO etapa 6).
    return nix({QStringLiteral("profile"), QStringLiteral("list"), QStringLiteral("--json")});
}

CliCommand NixAdapter::installCommand(const QString &packageId, const QString &) const
{
    return nix({QStringLiteral("profile"), QStringLiteral("install"),
                QStringLiteral("nixpkgs#") + packageId});
}

CliCommand NixAdapter::uninstallCommand(const QString &packageId, const QString &) const
{
    return nix({QStringLiteral("profile"), QStringLiteral("remove"), packageId});
}

CliCommand NixAdapter::upgradeCommand(const QString &packageId, const QString &) const
{
    return nix({QStringLiteral("profile"), QStringLiteral("upgrade"), packageId});
}

CliCommand NixAdapter::upgradeAllCommand() const
{
    return nix({QStringLiteral("profile"), QStringLiteral("upgrade"), QStringLiteral("--all")});
}

CliCommand NixAdapter::pinCommand(const QString &, const QString &) const
{
    return {}; // no per-package pin concept in nix profiles
}

CliCommand NixAdapter::unpinCommand(const QString &, const QString &) const
{
    return {};
}

QList<Package> NixAdapter::parseSearch(const QString &stdOut) const
{
    // {"legacyPackages.<system>.jq": {"pname": "jq", "version": "1.7.1",
    //                                 "description": "..."}}
    QList<Package> result;
    const QJsonObject root = QJsonDocument::fromJson(stdOut.toUtf8()).object();
    for (auto it = root.constBegin(); it != root.constEnd(); ++it) {
        const QJsonObject entry = it.value().toObject();
        Package p;
        p.id = attrToId(it.key());
        p.name = entry.value(QStringLiteral("pname")).toString(p.id);
        p.version = entry.value(QStringLiteral("version")).toString();
        p.description = entry.value(QStringLiteral("description")).toString();
        p.source = QStringLiteral("nixpkgs");
        result.append(p);
    }
    return result;
}

QList<Package> NixAdapter::parseInfo(const QString &stdOut) const
{
    return parseSearch(stdOut);
}

QList<Package> NixAdapter::parseInstalled(const QString &stdOut) const
{
    // nix profile list --json: {"elements": [...]} (array, older) or
    // {"elements": {"name": {...}}} (object keyed by name, newer).
    QList<Package> result;
    const QJsonObject root = QJsonDocument::fromJson(stdOut.toUtf8()).object();
    const QJsonValue elements = root.value(QStringLiteral("elements"));

    const auto toPackage = [](const QString &fallbackName, const QJsonObject &e) {
        Package p;
        const QString attrPath = e.value(QStringLiteral("attrPath")).toString();
        p.id = !fallbackName.isEmpty() ? fallbackName
                                       : (attrPath.isEmpty() ? QString() : attrToId(attrPath));
        const QJsonArray storePaths = e.value(QStringLiteral("storePaths")).toArray();
        if (!storePaths.isEmpty()) {
            const QString store = storePaths.first().toString();
            if (p.id.isEmpty()) {
                QString base = store.section(QLatin1Char('/'), -1);
                base.remove(0, base.indexOf(QLatin1Char('-')) + 1); // drop hash
                p.id = base;
            }
            p.installedVersion = versionFromStorePath(store);
        }
        p.name = p.id;
        p.version = p.installedVersion;
        p.source = e.value(QStringLiteral("originalUrl")).toString(QStringLiteral("nixpkgs"));
        return p;
    };

    if (elements.isArray()) {
        const QJsonArray array = elements.toArray();
        for (const auto &value : array) {
            const Package p = toPackage({}, value.toObject());
            if (!p.id.isEmpty())
                result.append(p);
        }
    } else if (elements.isObject()) {
        const QJsonObject object = elements.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            const Package p = toPackage(it.key(), it.value().toObject());
            if (!p.id.isEmpty())
                result.append(p);
        }
    }
    return result;
}

QList<Package> NixAdapter::parseOutdated(const QString &) const
{
    return {}; // see listOutdatedCommand()
}

QList<CliAction> NixAdapter::actionCatalog() const
{
    return {
        {QStringLiteral("rollback"), tr("Rollback profile"),
         tr("Return the profile to its previous generation"),
         nix({QStringLiteral("profile"), QStringLiteral("rollback")}), false, true, true},
        {QStringLiteral("history"), tr("Profile history"),
         tr("Show all generations of the profile with their changes"),
         nix({QStringLiteral("profile"), QStringLiteral("history")}), false, false, false},
        {QStringLiteral("gc"), tr("Garbage collect"),
         tr("Delete unreachable store paths to free disk space"),
         nix({QStringLiteral("store"), QStringLiteral("gc")}), false, true, false},
        {QStringLiteral("optimise"), tr("Optimise store"),
         tr("Deduplicate identical files in the nix store via hard links"),
         nix({QStringLiteral("store"), QStringLiteral("optimise")}), false, false, false},
        {QStringLiteral("registry"), tr("List flake registry"),
         tr("Show the flake registry entries (nixpkgs etc.)"),
         nix({QStringLiteral("registry"), QStringLiteral("list")}), false, false, false},
        {QStringLiteral("config"), tr("Show configuration"),
         tr("Print the effective nix configuration"),
         nix({QStringLiteral("config"), QStringLiteral("show")}), false, false, false},
    };
}
