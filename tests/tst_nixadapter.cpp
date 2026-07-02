#include <QTest>

#include "nixadapter.h"

class TestNixAdapter : public QObject {
    Q_OBJECT
private slots:
    void searchParsesJsonAttrKeys()
    {
        NixAdapter adapter;
        const auto packages = adapter.parseSearch(QStringLiteral(
            "{\"legacyPackages.aarch64-darwin.jq\": "
            "{\"pname\": \"jq\", \"version\": \"1.7.1\", "
            "\"description\": \"Command-line JSON processor\"},"
            "\"legacyPackages.aarch64-darwin.python312Packages.requests\": "
            "{\"pname\": \"python3.12-requests\", \"version\": \"2.32.3\", "
            "\"description\": \"HTTP library\"}}"));
        QCOMPARE(packages.size(), 2);
        // attr path keeps nested segments after the system prefix
        QStringList ids;
        for (const auto &p : packages)
            ids << p.id;
        QVERIFY(ids.contains(QStringLiteral("jq")));
        QVERIFY(ids.contains(QStringLiteral("python312Packages.requests")));
    }

    void profileListParsesArrayVariant()
    {
        NixAdapter adapter;
        const auto packages = adapter.parseInstalled(QStringLiteral(
            "{\"elements\": [{\"attrPath\": \"legacyPackages.aarch64-darwin.jq\","
            "\"originalUrl\": \"flake:nixpkgs\","
            "\"storePaths\": [\"/nix/store/abc123-jq-1.7.1\"]}], \"version\": 2}"));
        QCOMPARE(packages.size(), 1);
        QCOMPARE(packages.at(0).id, QStringLiteral("jq"));
        QCOMPARE(packages.at(0).installedVersion, QStringLiteral("1.7.1"));
    }

    void profileListParsesObjectVariant()
    {
        NixAdapter adapter;
        const auto packages = adapter.parseInstalled(QStringLiteral(
            "{\"elements\": {\"jq\": {\"storePaths\": "
            "[\"/nix/store/abc123-jq-1.7.1\"]}}, \"version\": 3}"));
        QCOMPARE(packages.size(), 1);
        QCOMPARE(packages.at(0).id, QStringLiteral("jq"));
        QCOMPARE(packages.at(0).installedVersion, QStringLiteral("1.7.1"));
    }

    void commandsEnableExperimentalFeaturesPerInvocation()
    {
        NixAdapter adapter;
        const auto cmd = adapter.installCommand("jq", "");
        QCOMPARE(cmd.program, QStringLiteral("nix"));
        QVERIFY(cmd.arguments.contains(QStringLiteral("--extra-experimental-features")));
        QCOMPARE(cmd.arguments.last(), QStringLiteral("nixpkgs#jq"));
        QVERIFY(!adapter.pinCommand("jq", "").isValid());
    }
};

QTEST_MAIN(TestNixAdapter)
#include "tst_nixadapter.moc"
