// Глобальные переменные
let sessionId = null;
let boardSize = 4;
let moves = 0;
let timerInterval = null;
let startTime = null;

// Элементы DOM
const gameBoard = document.getElementById('game-board');
const movesElement = document.getElementById('moves');
const timeElement = document.getElementById('time');
const difficultyElement = document.getElementById('difficulty');
const messageElement = document.getElementById('message');
const newGameBtn = document.getElementById('new-game-btn');
const statsBtn = document.getElementById('stats-btn');
const statsModal = document.getElementById('stats-modal');
const closeModal = document.querySelector('.close');

const hintBtn = document.getElementById('hint-btn');
const hintMessage = document.getElementById('hint-message');
const victoryModal = document.getElementById('victory-modal');
const closeVictory = document.querySelector('.close-victory');
const playAgainBtn = document.getElementById('play-again-btn');
const viewStatsBtn = document.getElementById('view-stats-btn');

const mainMenu = document.getElementById('main-menu');
const gameScreen = document.getElementById('game-screen');
const startGameBtn = document.getElementById('start-game-btn');
const difficultySelect = document.getElementById('difficulty-select');
const hintsToggle = document.getElementById('hints-toggle');
let selectedDifficulty = 'auto';
let hintsEnabled = true;


// Показать победное окно
function showVictory(data) {
    console.log('Showing victory modal...');

    try {
        document.getElementById('victory-moves').textContent = moves;
        document.getElementById('victory-time').textContent = timeElement.textContent;

        const efficiency = (data && data.efficiency) ? data.efficiency : 0;
        document.getElementById('victory-efficiency').textContent = (efficiency * 100).toFixed(1) + '%';

        // Определяем уровень мастерства
        let mastery = 'Новичок';
        if (efficiency > 0.8) mastery = 'Мастер';
        else if (efficiency > 0.6) mastery = 'Эксперт';
        else if (efficiency > 0.4) mastery = 'Продвинутый';
        else if (efficiency > 0.2) mastery = 'Средний';

        document.getElementById('victory-mastery').textContent = mastery;

        const victoryModal = document.getElementById('victory-modal');
        if (victoryModal) {
            victoryModal.style.display = 'block';
            console.log('✅ Victory modal displayed!');
        } else {
            console.error('❌ Victory modal element not found!');
        }
    } catch (error) {
        console.error('Error in showVictory:', error);
    }
}

// Обработчики для победного окна
document.addEventListener('DOMContentLoaded', () => {
    const closeVictory = document.querySelector('.close-victory');
    const playAgainBtn = document.getElementById('play-again-btn');
    const viewStatsBtn = document.getElementById('view-stats-btn');

    if (closeVictory) {
        closeVictory.addEventListener('click', () => {
            document.getElementById('victory-modal').style.display = 'none';
        });
    }

    if (playAgainBtn) {
        playAgainBtn.addEventListener('click', () => {
            document.getElementById('victory-modal').style.display = 'none';
            createNewGame();
        });
    }

    if (viewStatsBtn) {
        viewStatsBtn.addEventListener('click', () => {
            document.getElementById('victory-modal').style.display = 'none';
            showStats();
        });
    }

    // Закрытие по клику вне окна
    window.addEventListener('click', (e) => {
        const victoryModal = document.getElementById('victory-modal');
        if (e.target === victoryModal) {
            victoryModal.style.display = 'none';
        }
    });
});

function saveGameStats(data) {
    // Отправляем на сервер для сохранения
    fetch('/api/stats/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            moves: moves,
            time: timeElement.textContent,
            efficiency: data.efficiency || 0
        })
    });
}

function setupEventListeners() {
    newGameBtn.addEventListener('click', createNewGame);
    statsBtn.addEventListener('click', showStats);
    closeModal.addEventListener('click', () => {
        statsModal.style.display = 'none';
    });
    window.addEventListener('click', (e) => {
        if (e.target === statsModal) {
            statsModal.style.display = 'none';
        }
    });
    if (closeVictory) {
        closeVictory.addEventListener('click', () => {
            victoryModal.style.display = 'none';
        });
    }

    if (playAgainBtn) {
        playAgainBtn.addEventListener('click', () => {
            victoryModal.style.display = 'none';
            createNewGame();
        });
    }

    if (viewStatsBtn) {
        viewStatsBtn.addEventListener('click', () => {
            victoryModal.style.display = 'none';
            showStats();
        });
    }
    if (startGameBtn) {
        startGameBtn.addEventListener('click', () => {
            selectedDifficulty = difficultySelect.value;
            hintsEnabled = hintsToggle.checked;
            mainMenu.style.display = 'none';
            gameScreen.style.display = 'block';
            createNewGame();
        });
    }

    if (difficultySelect) {
        difficultySelect.addEventListener('change', (e) => {
            selectedDifficulty = e.target.value;
        });
    }

    if (hintsToggle) {
        hintsToggle.addEventListener('change', (e) => {
            hintsEnabled = e.target.checked;
            document.getElementById('hints-text').textContent =
                hintsEnabled ? 'Включены' : 'Выключены';
        });
    }
}

// Загрузка статистики меню
async function loadMenuStats() {
    try {
        const response = await fetch('/api/stats');
        const stats = await response.json();

        document.getElementById('menu-total-games').textContent = stats.totalGames || 0;
        document.getElementById('menu-wins').textContent = stats.completedGames || 0;
        document.getElementById('menu-efficiency').textContent =
            ((stats.avgEfficiency || 0) * 100).toFixed(1) + '%';

        // Определяем уровень мастерства
        const efficiency = stats.avgEfficiency || 0;
        let mastery = 'Новичок';
        let color = '#667eea';

        if (efficiency > 0.8) { mastery = '🏆 Мастер'; color = '#ffd700'; }
        else if (efficiency > 0.6) { mastery = '⭐ Эксперт'; color = '#38ef7d'; }
        else if (efficiency > 0.4) { mastery = '📈 Продвинутый'; color = '#667eea'; }
        else if (efficiency > 0.2) { mastery = '📚 Средний'; color = '#ffa500'; }
        else { mastery = '🌱 Новичок'; color = '#ff6b6b'; }

        const masteryEl = document.getElementById('mastery-level');
        masteryEl.textContent = mastery;
        masteryEl.style.color = color;

    } catch (error) {
        console.error('Ошибка загрузки статистики:', error);
    }
}

// Вызовите при загрузке страницы
document.addEventListener('DOMContentLoaded', () => {
    loadMenuStats();
    setupEventListeners();
});

// Загрузка конфигурации
async function loadConfig() {
    try {
        const response = await fetch('/api/config');
        const config = await response.json();
        difficultyElement.textContent = config.difficulty;
        boardSize = config.gridSize;
    } catch (error) {
        console.error('Ошибка загрузки конфигурации:', error);
    }
}


// Создание новой игры
async function createNewGame() {
    try {
        const response = await fetch('/api/game/create', {
            method: 'POST'
        });
        const data = await response.json();

        if (data.success) {
            sessionId = data.sessionId;
            boardSize = data.size;
            difficultyElement.textContent = data.difficulty || 'Средний';

            // Преобразуем плоский массив в двумерный
            const flatBoard = data.board;
            const board2D = [];
            for (let i = 0; i < boardSize; i++) {
                const row = [];
                for (let j = 0; j < boardSize; j++) {
                    row.push(flatBoard[i * boardSize + j]);
                }
                board2D.push(row);
            }

            renderBoard(board2D);
            moves = 0;
            movesElement.textContent = moves;
            startTimer();
            showMessage('', '');
        } else {
            showMessage('Ошибка создания игры', 'error');
        }
    } catch (error) {
        console.error('Ошибка:', error);
        showMessage('Ошибка соединения с сервером', 'error');
    }
}


// Отрисовка игрового поля
function renderBoard(board) {
    gameBoard.innerHTML = '';
    gameBoard.style.gridTemplateColumns = `repeat(${boardSize}, 1fr)`;

    console.log('Rendering board:', board); // Для отладки

    for (let i = 0; i < boardSize; i++) {
        for (let j = 0; j < boardSize; j++) {
            const tile = document.createElement('button');
            tile.className = 'tile';
            const value = board[i][j];

            console.log(`Tile [${i}][${j}] = ${value}`); // Для отладки

            if (value === 0) {
                // Пустая клетка
                tile.classList.add('empty');
                tile.disabled = true;
                tile.textContent = '';
            } else {
                // Плитка с номером
                tile.textContent = value.toString();
                tile.dataset.row = i;
                tile.dataset.col = j;
                tile.addEventListener('click', () => makeMove(i, j));

                // Проверка правильной позиции
                const correctPos = (value - 1);
                const correctRow = Math.floor(correctPos / boardSize);
                const correctCol = correctPos % boardSize;
                if (correctRow === i && correctCol === j) {
                    tile.classList.add('correct');
                }
            }

            gameBoard.appendChild(tile);
        }
    }
}

// Сделать ход
async function makeMove(row, col) {
    console.log('makeMove called with:', row, col);

    if (!sessionId) {
        console.error('No session ID!');
        alert('Сначала начните новую игру!');
        return;
    }

    try {
        const response = await fetch('/api/game/move', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ sessionId, row, col })
        });

        console.log('Response status:', response.status);

        // Проверяем, успешен ли ответ
        if (!response.ok) {
            console.error('Server error:', response.status);
            showMessage('Ошибка сервера', 'error');
            return;
        }

        const data = await response.json();
        console.log('Response data:', data);

        if (!data) {
            console.error('Empty response data');
            showMessage('Пустой ответ от сервера', 'error');
            return;
        }

        if (data.success) {
            // Преобразуем плоский массив в двумерный
            if (data.board) {
                const flatBoard = data.board;
                const board2D = [];
                for (let i = 0; i < boardSize; i++) {
                    const row = [];
                    for (let j = 0; j < boardSize; j++) {
                        row.push(flatBoard[i * boardSize + j]);
                    }
                    board2D.push(row);
                }

                renderBoard(board2D);
                moves = data.moves || 0;
                movesElement.textContent = moves;

                // Проверяем победу
                if (data.solved) {
                    console.log('🎉 PUZZLE SOLVED!');
                    stopTimer();
                    showVictory(data);
                }
            } else {
                console.error('No board in response');
            }
        } else {
            console.log('Move not allowed');
            showMessage('Недопустимый ход', 'error');
        }
    } catch (error) {
        console.error('Ошибка хода:', error);
        showMessage('Ошибка соединения с сервером', 'error');
    }
}

// Таймер
function startTimer() {
    startTime = Date.now();
    if (timerInterval) clearInterval(timerInterval);
    timerInterval = setInterval(updateTimer, 1000);
}

function updateTimer() {
    const elapsed = Math.floor((Date.now() - startTime) / 1000);
    const minutes = Math.floor(elapsed / 60).toString().padStart(2, '0');
    const seconds = (elapsed % 60).toString().padStart(2, '0');
    timeElement.textContent = `${minutes}:${seconds}`;
}

function stopTimer() {
    if (timerInterval) {
        clearInterval(timerInterval);
        timerInterval = null;
    }
}

// Показать сообщение
// Показать сообщение
function showMessage(text, type) {
    const messageElement = document.getElementById('message');
    const hintMessage = document.getElementById('hint-message');

    // Используем hint-message если есть, иначе message
    const targetElement = hintMessage || messageElement;

    if (targetElement) {
        targetElement.textContent = text;
        targetElement.className = 'message ' + type;
        targetElement.style.display = 'block';

        // Скрываем через 5 секунд
        setTimeout(() => {
            targetElement.style.display = 'none';
        }, 5000);
    } else {
        console.log('Message:', text, type);
    }
}

// Показать статистику
async function showStats() {
    try {
        const response = await fetch('/api/stats');
        const stats = await response.json();

        const content = document.getElementById('stats-content');
        content.innerHTML = `
            <div class="stat-row">
                <span class="stat-label">Всего игр:</span>
                <span class="stat-value">${stats.totalGames}</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Завершено:</span>
                <span class="stat-value">${stats.completedGames}</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Средняя эффективность:</span>
                <span class="stat-value">${(stats.avgEfficiency * 100).toFixed(1)}%</span>
            </div>
            <div class="stat-row">
                <span class="stat-label">Среднее время:</span>
                <span class="stat-value">${stats.avgTime.toFixed(1)} сек</span>
            </div>
            <div class="stat-row" style="border-bottom: none;">
                <span class="stat-label">Рекомендация:</span>
            </div>
            <div style="margin-top: 15px; padding: 15px; background: #f8f9fa; border-radius: 10px; color: #333;">
                ${stats.recommendation}
            </div>
        `;

        statsModal.style.display = 'block';
    } catch (error) {
        console.error('Ошибка загрузки статистики:', error);
        showMessage('Ошибка загрузки статистики', 'error');
    }
}


hintBtn.addEventListener('click', async () => {
    try {
        const response = await fetch('/api/hint');
        const data = await response.json();

        if (data.success) {
            hintMessage.textContent = data.description;
            hintMessage.className = 'message hint';
            hintMessage.style.display = 'block';

            // Подсветить плитку
            highlightTile(data.fromRow, data.fromCol);
        }
    } catch (error) {
        console.error('Ошибка подсказки:', error);
    }
});

function highlightTile(row, col) {
    const tiles = document.querySelectorAll('.tile');
    tiles.forEach(tile => {
        if (tile.dataset.row == row && tile.dataset.col == col) {
            tile.style.animation = 'pulse 0.5s ease 3';
            setTimeout(() => {
                tile.style.animation = '';
            }, 1500);
        }
    });
}


