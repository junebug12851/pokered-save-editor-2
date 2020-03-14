#include <QApplication>

extern QApplication* boot(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    // Bootstrap, Setup, Load, Prep, and Run the app
    return boot(argc, argv)->exec();
}
