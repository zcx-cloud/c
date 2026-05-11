#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QPixmap>
#include <QVector>
#include <random>

class GameWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget *parent = nullptr);
    ~GameWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    // 移动物体结构
    struct MovingObject {
        int x, y;
        int size;
        int speedX, speedY;
        int imageIndex;
        QPixmap pixmap;
        double distance;
        int health;
        int maxHealth;
        bool isDead;

        MovingObject() : x(0), y(0), size(0), speedX(0), speedY(0),
            imageIndex(-1), distance(0.0), health(100),
            maxHealth(100), isDead(false) {}
    };



    void initGame();


    bool loadImages();
    void updateBackground();


    double calculateDistance(int x1, int y1, int x2, int y2);
    int randomInt(int min, int max);
    QColor randomColor();
    int getRandomSpeed();
    void checkBoundary(int &x, int &y, int size);
    bool checkCollision(int x1, int y1, int size1, int x2, int y2, int size2);


    void updateGame();
    void updatePlayerPosition();
    void updateRandomObjects();
    void calculateDistances();
    int findNearestObject();
    void addRandomObject();
    void removeRandomObject();
    void autoAttack();
    void handleCollisions();
    void checkObjectDeath();


    void checkAndUpgradeAttack();


    void drawBackground(QPainter &painter);
    void drawGrid(QPainter &painter);
    void drawPlayer(QPainter &painter);
    void drawRandomObjects(QPainter &painter);
    void drawConnection(QPainter &painter);
    void drawInfoPanel(QPainter &painter);
    void drawHealthBar(QPainter &painter, int x, int y, int width, int height,
                       int currentHealth, int maxHealth);

    void drawTextWithOutline(QPainter &painter, int x, int y, const QString &text,
                             const QColor &textColor, const QColor &outlineColor, int outlineWidth);

    int playerX, playerY;
    int playerSize;
    int playerHealth;
    int playerMaxHealth;
    int playerAttack;
    int moveSpeed;
    float attackCooldown;
    float attackTimer;
    int killCount;
    int killsForUpgrade;
    int upgradeLevel;
    int attackBonus;
    int totalKills;

    bool moveUp, moveDown, moveLeft, moveRight;


    bool showConnection;
    bool showGrid;
    bool showHealthBars;
    float backgroundOpacity;


    QVector<MovingObject> movingObjects;

    int nearestObjectIndex;
    double nearestDistance;


    QPixmap backgroundPixmap;
    QPixmap scaledBackground;
    QPixmap playerPixmap;
    QVector<QPixmap> objectImages;


    QTimer *gameTimer;


    std::random_device rd;
    std::mt19937 gen;
};

#endif // GAMEWINDOW_H
