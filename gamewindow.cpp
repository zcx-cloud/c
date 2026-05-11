#include "gamewindow.h"
#include <QPainter>
#include <QDebug>
#include <QMouseEvent>
#include <QPen>
#include <QBrush>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QFont>
#include <QMessageBox>
#include <QResizeEvent>
#include <cmath>

GameWindow::GameWindow(QWidget *parent) : QWidget(parent)
    , attackCooldown(0.2f)
    , attackTimer(0.0f)

    , bossPointer(nullptr)
{
    setFixedSize(1417, 670);

    if (!loadImages()) {
        QMessageBox::warning(this, "警告", "无法加载图片资源！");
    }

    initGame();

    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &GameWindow::updateGame);
    gameTimer->start(16);
}

GameWindow::~GameWindow()
{
    delete gameTimer;
}

bool GameWindow::loadImages()
{
    objectImages.clear();
    backgroundPixmap.load(":/cailiao/beijing.png");
    updateBackground();
    playerPixmap.load(":/cailiao/baima.png");

    QStringList imageFiles = {
        ":/cailiao/guai.png",
        ":/cailiao/lantern.png",
        ":/cailiao/boss.png"
    };

    for (const QString &file : imageFiles) {
        QPixmap pixmap(file);
        objectImages.append(pixmap);
    }
    return true;
}

void GameWindow::updateBackground()
{
    if (!backgroundPixmap.isNull()) {
        scaledBackground = backgroundPixmap.scaled(size(),
                                                   Qt::IgnoreAspectRatio,
                                                   Qt::SmoothTransformation);
    }
}

double GameWindow::calculateDistance(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

void GameWindow::initGame()
{
    playerX = 400;
    playerY = 300;
    playerSize = 50;
    playerHealth = 500;
    playerMaxHealth = 500;
    playerAttack = 10;
    moveSpeed = 5;
    attackTimer = 0.0f;
    killCount = 0;
    killsForUpgrade = 3;
    upgradeLevel = 0;
    attackBonus = 0;
    totalKills = 0;

    moveUp = false;
    moveDown = false;
    moveLeft = false;
    moveRight = false;

    showConnection = true;
    showHealthBars = true;
    nearestObjectIndex = -1;
    nearestDistance = 0.0;
    backgroundOpacity = 0.8;
    gameVictory = false;


    bossSpawned = false;
    bossSpawnTimer = 0.0f;
    bossSpawnDelay = 30.0f;
    bossIndex = -1;
    bossPointer = nullptr;
    bossShootTimer = 0.0f;
    bossShootDelay = 1.0f;


    bullets.clear();
    movingObjects.clear();


    for (int i = 0; i < 8; i++) {
        addRandomObject();
    }
}

int GameWindow::randomInt(int min, int max)
{
    std::uniform_int_distribution<> distrib(min, max);
    return distrib(gen);
}

QColor GameWindow::randomColor()
{
    int r = randomInt(50, 255);
    int g = randomInt(50, 255);
    int b = randomInt(50, 255);
    return QColor(r, g, b);
}

int GameWindow::getRandomSpeed()
{
    float speed = randomInt(1, 2);
    return randomInt(0, 1) ? speed : -speed;
}

void GameWindow::addRandomObject()
{
    MovingObject obj;
    obj.size = randomInt(40, 70);
    obj.isBoss = false;
    obj.imageIndex = randomInt(0, 1);
    obj.pixmap = objectImages[obj.imageIndex];


    if (obj.size >= 60) {
        obj.health = 150;
        obj.maxHealth = 150;
    } else if (obj.size >= 50) {
        obj.health = 100;
        obj.maxHealth = 100;
    } else {
        obj.health = 60;
        obj.maxHealth = 60;
    }
    obj.isDead = false;

    if (!obj.pixmap.isNull()) {
        obj.pixmap = obj.pixmap.scaled(obj.size, obj.size,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
    }


    obj.x = randomInt(obj.size, width() - obj.size);
    obj.y = randomInt(obj.size, height() - obj.size);


    obj.speedX = getRandomSpeed();
    obj.speedY = getRandomSpeed();

    movingObjects.append(obj);
}

void GameWindow::removeRandomObject()
{
    if (!movingObjects.isEmpty()) {
        movingObjects.removeLast();
    }
}

void GameWindow::spawnBoss()
{
    if (bossSpawned) return;

    MovingObject boss;
    boss.size = 100;
    boss.isBoss = true;
    boss.health = 500;
    boss.maxHealth = 500;
    boss.isDead = false;
    boss.imageIndex = 2;
    boss.pixmap = objectImages[2];

    if (!boss.pixmap.isNull()) {
        boss.pixmap = boss.pixmap.scaled(boss.size, boss.size,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    }


    boss.x = width() - 150;
    boss.y = 100;


    boss.speedX = 2;
    boss.speedY = 2;

    movingObjects.append(boss);
    bossIndex = movingObjects.size() - 1;
    bossPointer = &movingObjects[bossIndex];
    bossSpawned = true;


}

void GameWindow::updateBoss()
{

    if (!bossSpawned || bossIndex < 0 || bossIndex >= movingObjects.size()) {
        return;
    }


    if (bossPointer == nullptr || bossPointer->isBoss == false) {

        for (int i = 0; i < movingObjects.size(); i++) {
            if (movingObjects[i].isBoss) {
                bossIndex = i;
                bossPointer = &movingObjects[i];
                break;
            }
        }
        if (bossPointer == nullptr) return;
    }

    MovingObject &boss = *bossPointer;


    if (boss.isDead || boss.health <= 0) {
        return;
    }


    int dx = playerX - boss.x;
    int dy = playerY - boss.y;
    double distanceToPlayer = calculateDistance(boss.x, boss.y, playerX, playerY);

    if (distanceToPlayer > 0) {

        float healthPercent = (float)boss.health / boss.maxHealth;
        float baseSpeed = 3.0f;
        float speedMultiplier = 1.0f + (1.0f - healthPercent) * 1.5f;
        float currentSpeed = baseSpeed * speedMultiplier;


        boss.speedX = (dx / distanceToPlayer) * currentSpeed;
        boss.speedY = (dy / distanceToPlayer) * currentSpeed;


        boss.x += boss.speedX;
        boss.y += boss.speedY;


        if (boss.x < boss.size/2) {
            boss.x = boss.size/2;
            boss.speedX = abs(boss.speedX);
        }
        if (boss.x > width() - boss.size/2) {
            boss.x = width() - boss.size/2;
            boss.speedX = -abs(boss.speedX);
        }
        if (boss.y < boss.size/2) {
            boss.y = boss.size/2;
            boss.speedY = abs(boss.speedY);
        }
        if (boss.y > height() - boss.size/2) {
            boss.y = height() - boss.size/2;
            boss.speedY = -abs(boss.speedY);
        }
    }


    if (bossShootTimer <= 0) {
        bossShoot();
        float healthPercent = (float)boss.health / boss.maxHealth;
        if (healthPercent < 0.3f) {
            bossShootDelay = 0.5f;
        } else if (healthPercent < 0.6f) {
            bossShootDelay = 0.8f;
        } else {
            bossShootDelay = 1.0f;
        }

        bossShootTimer = bossShootDelay;
    } else {
        bossShootTimer -= 0.016f;
    }
}

void GameWindow::bossShoot()
{
    if (!bossSpawned || bossPointer == nullptr) return;

    MovingObject &boss = *bossPointer;
    if (boss.isDead || boss.health <= 0) return;


    int dx = playerX - boss.x;
    int dy = playerY - boss.y;
    double distance = calculateDistance(boss.x, boss.y, playerX, playerY);

    if (distance > 0) {
        float speed = 6.0f;
        float healthPercent = (float)boss.health / boss.maxHealth;
        int bulletCount = 1;

        if (healthPercent < 0.3f) {
            bulletCount = 5;
        } else if (healthPercent < 0.6f) {
            bulletCount = 3;
        }


        int speedX = (dx / distance) * speed;
        int speedY = (dy / distance) * speed;

        for (int i = 0; i < bulletCount; i++) {
            Bullet bullet;
            bullet.x = boss.x;
            bullet.y = boss.y;
            bullet.size = (i == 0) ? 10 : 8;
            bullet.active = true;

            if (i == 0) {
                bullet.speedX = speedX;
                bullet.speedY = speedY;
            } else {
                float angleOffset = (i - bulletCount/2.0f) * 0.4f;
                double angle = atan2(dy, dx);
                angle += angleOffset;
                bullet.speedX = cos(angle) * (speed - 1);
                bullet.speedY = sin(angle) * (speed - 1);
            }

            bullets.append(bullet);
        }


        if (healthPercent < 0.3f) {
            for (int i = 0; i < 8; i++) {
                Bullet ringBullet;
                ringBullet.x = boss.x;
                ringBullet.y = boss.y;
                ringBullet.size = 6;
                ringBullet.active = true;

                double angle = (i * 45.0) * 3.14159 / 180.0;
                ringBullet.speedX = cos(angle) * 5;
                ringBullet.speedY = sin(angle) * 5;
                bullets.append(ringBullet);
            }
        }
    }
}

void GameWindow::updateBullets()
{
    for (int i = bullets.size() - 1; i >= 0; i--) {
        Bullet &bullet = bullets[i];

        bullet.x += bullet.speedX;
        bullet.y += bullet.speedY;


        if (bullet.x < 0 || bullet.x > width() ||
            bullet.y < 0 || bullet.y > height()) {
            bullets.removeAt(i);
            continue;
        }


        if (checkCollision(bullet.x, bullet.y, bullet.size/2,
                           playerX, playerY, playerSize/2)) {
            int damage = 15;
            playerHealth -= damage;
            bullets.removeAt(i);

            if (playerHealth <= 0) {
                playerHealth = 0;
                gameTimer->stop();
                QMessageBox::information(this, "游戏结束", "你被击败了！");
            }
        }
    }
}


void GameWindow::updateGame()
{
    if (gameVictory) return;


    if (!bossSpawned) {
        bossSpawnTimer += 0.016f;
        if (bossSpawnTimer >= bossSpawnDelay) {
            spawnBoss();
        }
    }

    if (attackTimer > 0) {
        attackTimer -= 0.016f;
    }

    updatePlayerPosition();
    updateRandomObjects();
    updateBoss();
    updateBullets();
    handleCollisions();
    autoAttack();
    checkObjectDeath();
    checkAndUpgradeAttack();


    if (!movingObjects.isEmpty()) {
        calculateDistances();
        nearestObjectIndex = findNearestObject();
        if (nearestObjectIndex >= 0) {
            nearestDistance = movingObjects[nearestObjectIndex].distance;
        }
    } else {
        nearestObjectIndex = -1;
        nearestDistance = 0.0;
    }

    update();
}

void GameWindow::autoAttack()
{
    if (attackTimer > 0) return;

    int nearestEnemy = findNearestObject();
    if (nearestEnemy >= 0 && nearestDistance < 200) {
        MovingObject &enemy = movingObjects[nearestEnemy];
        enemy.health -= playerAttack;
        attackTimer = attackCooldown;

        if (enemy.health <= 0) {
            enemy.isDead = true;
        }
    }
}

void GameWindow::handleCollisions()
{
    for (int i = 0; i < movingObjects.size(); i++) {
        MovingObject &obj = movingObjects[i];

        if (obj.isDead) continue;

        if (checkCollision(playerX, playerY, playerSize/2, obj.x, obj.y, obj.size/2)) {
            int damage = obj.isBoss ? 30 : 10;
            playerHealth -= damage;

            if (playerHealth <= 0) {
                playerHealth = 0;
                gameTimer->stop();
                QMessageBox::information(this, "游戏结束", "你被击败了！");
            }



            break;
        }
    }
}

void GameWindow::checkObjectDeath()
{
    for (int i = movingObjects.size() - 1; i >= 0; i--) {
        if (movingObjects[i].isDead || movingObjects[i].health <= 0) {

            if (movingObjects[i].isBoss) {

                bossPointer = nullptr;
                bossIndex = -1;
            }

            movingObjects.removeAt(i);
            killCount++;
            totalKills++;
        }
    }


    if (bossSpawned && (bossPointer == nullptr || bossIndex == -1)) {
        for (int i = 0; i < movingObjects.size(); i++) {
            if (movingObjects[i].isBoss) {
                bossIndex = i;
                bossPointer = &movingObjects[i];
                break;
            }
        }
    }


    if (!bossSpawned && movingObjects.size() < 5) {
        int addCount = 2 + upgradeLevel / 3;
        for (int i = 0; i < addCount; i++) {
            addRandomObject();
        }
    }
}

void GameWindow::checkAndUpgradeAttack()
{
    while (killCount >= killsForUpgrade) {
        killCount -= killsForUpgrade;
        upgradeLevel++;

        int attackIncrease = 2;
        playerAttack += attackIncrease;
        killsForUpgrade += 2;
        int healAmount = 50;
        playerHealth = qMin(playerHealth + healAmount, playerMaxHealth);


    }
}

void GameWindow::checkBoundary(int &x, int &y, int size)
{
    if (x < size/2) {
        x = size/2;
    } else if (x > width() - size/2) {
        x = width() - size/2;
    }

    if (y < size/2) {
        y = size/2;
    } else if (y > height() - size/2) {
        y = height() - size/2;
    }
}

void GameWindow::updatePlayerPosition()
{
    if (moveUp) playerY -= moveSpeed;
    if (moveDown) playerY += moveSpeed;
    if (moveLeft) playerX -= moveSpeed;
    if (moveRight) playerX += moveSpeed;

    checkBoundary(playerX, playerY, playerSize);
}

void GameWindow::updateRandomObjects()
{
    for (int i = 0; i < movingObjects.size(); i++) {
        MovingObject &obj = movingObjects[i];


        if (obj.isBoss) continue;

        obj.x += obj.speedX;
        obj.y += obj.speedY;

        if (obj.x <= obj.size/2 || obj.x >= width() - obj.size/2) {
            obj.speedX = -obj.speedX;
            if (obj.x <= obj.size/2) obj.x = obj.size/2 + 1;
            if (obj.x >= width() - obj.size/2) obj.x = width() - obj.size/2 - 1;
        }

        if (obj.y <= obj.size/2 || obj.y >= height() - obj.size/2) {
            obj.speedY = -obj.speedY;
            if (obj.y <= obj.size/2) obj.y = obj.size/2 + 1;
            if (obj.y >= height() - obj.size/2) obj.y = height() - obj.size/2 - 1;
        }

        if (randomInt(0, 100) < 2) {
            obj.speedX = getRandomSpeed();
        }

        if (randomInt(0, 100) < 2) {
            obj.speedY = getRandomSpeed();
        }
    }
}

void GameWindow::calculateDistances()
{
    for (int i = 0; i < movingObjects.size(); i++) {
        MovingObject &obj = movingObjects[i];
        obj.distance = calculateDistance(playerX, playerY, obj.x, obj.y);
    }
}

int GameWindow::findNearestObject()
{
    if (movingObjects.isEmpty()) {
        return -1;
    }

    int nearestIndex = 0;
    double minDistance = movingObjects[0].distance;

    for (int i = 1; i < movingObjects.size(); i++) {
        if (movingObjects[i].distance < minDistance) {
            minDistance = movingObjects[i].distance;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

bool GameWindow::checkCollision(int x1, int y1, int size1, int x2, int y2, int size2)
{
    int dx = x1 - x2;
    int dy = y1 - y2;
    int distanceSquared = dx * dx + dy * dy;
    int radiusSum = size1 + size2;

    return distanceSquared <= radiusSum * radiusSum;
}

void GameWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);
    drawRandomObjects(painter);
    drawBullets(painter);
    drawPlayer(painter);

    if (showConnection && nearestObjectIndex >= 0) {
        drawConnection(painter);
    }

    drawInfoPanel(painter);

    if (!bossSpawned && !gameVictory) {
        painter.save();
        QFont font = painter.font();
        font.setPointSize(20);
        font.setBold(true);
        painter.setFont(font);
        int remainingTime = ceil(bossSpawnDelay - bossSpawnTimer);
        drawTextWithOutline(painter, width()/2 - 100, 100,
                            QString("Boss出现: %1秒").arg(remainingTime),
                            Qt::red, Qt::black, 2);
        painter.restore();
    }
}

void GameWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateBackground();
}

void GameWindow::drawBackground(QPainter &painter)
{
    if (!scaledBackground.isNull()) {
        painter.drawPixmap(0, 0, scaledBackground);
    }
}

void GameWindow::drawPlayer(QPainter &painter)
{
    painter.save();

    if (!playerPixmap.isNull()) {
        painter.drawPixmap(playerX - playerSize/2, playerY - playerSize/2,
                           playerSize, playerSize, playerPixmap);
    }

    if (showHealthBars) {
        int barWidth = playerSize;
        int barHeight = 8;
        int barX = playerX - barWidth/2;
        int barY = playerY - playerSize/2 - 15;
        drawHealthBar(painter, barX, barY, barWidth, barHeight,
                      playerHealth, playerMaxHealth);
    }

    painter.restore();
}

void GameWindow::drawRandomObjects(QPainter &painter)
{
    for (int i = 0; i < movingObjects.size(); i++) {
        const MovingObject &obj = movingObjects[i];
        painter.save();

        if (!obj.pixmap.isNull()) {
            painter.drawPixmap(obj.x - obj.size/2, obj.y - obj.size/2,
                               obj.size, obj.size, obj.pixmap);
        }


        if (obj.isBoss) {
            painter.setBrush(QColor(255, 0, 0, 50));
            painter.setPen(QPen(Qt::red, 3));
            painter.drawEllipse(obj.x - obj.size/2 - 5, obj.y - obj.size/2 - 5,
                                obj.size + 10, obj.size + 10);
        }

        if (showHealthBars) {
            int barWidth = obj.size;
            int barHeight = 8;
            int barX = obj.x - barWidth/2;
            int barY = obj.y - obj.size/2 - 10;
            drawHealthBar(painter, barX, barY, barWidth, barHeight,
                          obj.health, obj.maxHealth);
        }

        painter.restore();
    }
}

void GameWindow::drawBullets(QPainter &painter)
{
    painter.save();

    for (const Bullet &bullet : bullets) {
        if (!bullet.active) continue;

        painter.setBrush(QColor(255, 50, 50, 200));
        painter.setPen(QPen(Qt::red, 2));
        painter.drawEllipse(bullet.x - bullet.size/2, bullet.y - bullet.size/2,
                            bullet.size, bullet.size);

        painter.setBrush(QColor(255, 0, 0, 100));
        painter.drawEllipse(bullet.x - bullet.size/4, bullet.y - bullet.size/4,
                            bullet.size/2, bullet.size/2);
    }

    painter.restore();
}

void GameWindow::drawHealthBar(QPainter &painter, int x, int y, int width, int height,
                               int currentHealth, int maxHealth)
{
    float healthPercent = (float)currentHealth / maxHealth;
    healthPercent = qBound(0.0f, healthPercent, 1.0f);
    int filledWidth = width * healthPercent;

    painter.setBrush(QColor(100, 0, 0, 200));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(x, y, width, height);

    QColor healthColor;
    if (healthPercent > 0.6f) {
        healthColor = QColor(0, 255, 0);
    } else if (healthPercent > 0.3f) {
        healthColor = QColor(255, 255, 0);
    } else {
        healthColor = QColor(255, 0, 0);
    }

    painter.setBrush(healthColor);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x, y, filledWidth, height);
}

void GameWindow::drawConnection(QPainter &painter)
{
    if (nearestObjectIndex < 0 || nearestObjectIndex >= movingObjects.size()) return;

    const MovingObject &nearestObj = movingObjects[nearestObjectIndex];
    painter.save();

    float maxConnectionDistance = 200.0f;

    if (nearestDistance <= maxConnectionDistance) {
        QColor lineColor;
        if (nearestDistance < 100) {
            lineColor = QColor(255, 0, 0, 200);
        } else if (nearestDistance < 200) {
            lineColor = QColor(255, 255, 0, 200);
        } else {
            lineColor = QColor(0, 255, 0, 200);
        }

        int alpha = 255;
        if (nearestDistance > 150) {
            alpha = 255 * (1 - (nearestDistance - 150) / 50);
            lineColor.setAlpha(alpha);
        }

        QPen linePen(lineColor, 3);
        linePen.setStyle(Qt::SolidLine);
        painter.setPen(linePen);
        painter.drawLine(playerX, playerY, nearestObj.x, nearestObj.y);
    }

    painter.restore();
}

void GameWindow::drawTextWithOutline(QPainter &painter, int x, int y, const QString &text,
                                     const QColor &textColor, const QColor &outlineColor, int outlineWidth)
{
    painter.setPen(QPen(outlineColor, outlineWidth));
    painter.drawText(x-1, y-1, text);
    painter.drawText(x,   y-1, text);
    painter.drawText(x+1, y-1, text);
    painter.drawText(x-1, y,   text);
    painter.drawText(x+1, y,   text);
    painter.drawText(x-1, y+1, text);
    painter.drawText(x,   y+1, text);
    painter.drawText(x+1, y+1, text);
    painter.setPen(textColor);
    painter.drawText(x, y, text);
}

void GameWindow::drawInfoPanel(QPainter &painter)
{


    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(10);
    painter.setFont(font);

    int yPos = 30;
    int lineHeight = 22;

    drawTextWithOutline(painter, 15, yPos, "=== 玩家状态 ===", Qt::yellow, Qt::black, 1);
    yPos += lineHeight;
    drawTextWithOutline(painter, 15, yPos, QString("血量: %1 / %2").arg(playerHealth).arg(playerMaxHealth),
                        Qt::white, Qt::black, 1);
    yPos += lineHeight;
    drawTextWithOutline(painter, 15, yPos, QString("攻击力: %1").arg(playerAttack), Qt::white, Qt::black, 1);
    yPos += lineHeight;
    drawTextWithOutline(painter, 15, yPos, QString("击杀数: %1 / %2").arg(killCount).arg(killsForUpgrade),
                        Qt::white, Qt::black, 1);
    yPos += lineHeight;
    drawTextWithOutline(painter, 15, yPos, QString("等级: %1").arg(upgradeLevel), Qt::white, Qt::black, 1);
    yPos += lineHeight;

    if (bossSpawned && bossPointer != nullptr && !bossPointer->isDead) {
        drawTextWithOutline(painter, 15, yPos, "=== Boss状态 ===", Qt::red, Qt::black, 1);
        yPos += lineHeight;

        float healthPercent = (float)bossPointer->health / bossPointer->maxHealth;

        drawTextWithOutline(painter, 15, yPos,
                            QString("Boss血量: %1 / %2").arg(bossPointer->health).arg(bossPointer->maxHealth),
                            Qt::red, Qt::black, 1);
        yPos += lineHeight;

        QString phase;
        if (healthPercent < 0.3f) {
            phase = "濒死阶段 (疯狂模式)";
        } else if (healthPercent < 0.6f) {
            phase = "受伤阶段 (强化模式)";
        } else {
            phase = "正常阶段";
        }
        drawTextWithOutline(painter, 15, yPos, QString("Boss阶段: %1").arg(phase),
                            QColor(255, 165, 0), Qt::black, 1);

    }

    painter.restore();
}

void GameWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        moveUp = true;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        moveDown = true;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        moveLeft = true;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        moveRight = true;
        break;

    }
}

void GameWindow::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        moveUp = false;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        moveDown = false;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        moveLeft = false;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        moveRight = false;
        break;
    }
}

void GameWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        addRandomObject();
    } else if (event->button() == Qt::RightButton) {
        showConnection = !showConnection;
    }
}
