#include "mainwindow.h"
#include <QApplication>

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QtAndroid>
#endif

static int androidSdkInt() {
#ifdef Q_OS_ANDROID
    return QAndroidJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");
#else
    return 0;
#endif
}

static void requestAndroidBtPermissions() {
#ifdef Q_OS_ANDROID
    QStringList perms;
    const int sdk = androidSdkInt();
    if (sdk >= 31) {
        perms << "android.permission.BLUETOOTH_CONNECT"
              << "android.permission.BLUETOOTH_SCAN";
    } else {
        perms << "android.permission.BLUETOOTH"
              << "android.permission.BLUETOOTH_ADMIN"
              << "android.permission.ACCESS_COARSE_LOCATION"
              << "android.permission.ACCESS_FINE_LOCATION";
    }
    QtAndroid::requestPermissionsSync(perms);
#endif
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
#ifdef Q_OS_ANDROID
    requestAndroidBtPermissions();
#endif
    MainWindow w;
    w.show();
    return app.exec();
}
