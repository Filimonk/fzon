class Header {
    constructor() {
        this.cartCount; // Данные по умолчанию 0 выставляются при инициализации
        this.username; // Данные по умолчанию 'Вход' выставляются при инициализации
        this.authStatus; // Данные по умолчанию 0 выставляются при инициализации
        this.authModal = null;
        this.token = localStorage.getItem('jwt_token');
    }

    async initialize() {
        // Проверяем авторизацию при инициализации
        if (this.token) {
            try {
                await this.fetchUserData();
                return;
            } catch (error) {
                console.error('Ошибка при получении данных пользователя:', error);
            }
        }
        
        this.setCartCount(0); // Данные по умолчанию
        this.setUsername('Вход'); // Данные по умолчанию
        this.setAuthStatus(0); // Данные по умолчанию
    }

    async fetchUserData() {
        const response = await fetch('/api/user-profiler/get-header-data', {
            headers: {
                'Authorization': `Bearer ${this.token}`
            }
        });

        if (response.ok) {
            const data = await response.json();
            this.setCartCount(data.cartCount);
            this.setUsername(data.username);
            this.setAuthStatus(1);
            return;
        }

        if (response.status === 401 || response.status === 403) {
            // Токен недействителен, удаляем его
            localStorage.removeItem('jwt_token');
            this.token = null;
            throw new Error('Токен недействителен');
        }

        throw new Error(`Ошибка сервера: ${response.status}`);
    }

    setCartCount(count) {
        this.cartCount = count;
        this.updateCartCount();
    }

    setUsername(name) {
        this.username = name;
        this.updateUsername();
    }

    setAuthStatus(status_var) {
        this.authStatus = status_var;
    }

    updateCartCount() {
        const cartCountElement = document.querySelector('.cart-count');
        if (cartCountElement) {
            if (this.cartCount > 0) {
                cartCountElement.textContent = this.cartCount;
                cartCountElement.style.display = 'block';

                cartCountElement.classList.remove('multi-digit');
                if (this.cartCount > 9) {
                    cartCountElement.classList.add('multi-digit');
                }
            } else {
                cartCountElement.style.display = 'none';
            }
        }
    }

    updateUsername() {
        const loginElement = document.querySelector('.nav-item.login span');
        if (loginElement && this.username) {
            loginElement.textContent = this.username;
        }
    }

    showAuthModal() {
        if (!this.authModal) {
            this.authModal = new AuthModal(this);
        }
        this.authModal.show();
    }

    handleLoginSuccess(token, userData) {
        localStorage.setItem('jwt_token', token);
        this.token = token;
        this.initialize();

        if (this.authModal) {
            this.authModal.hide();
        }
    }

    render(containerId) {
        const container = document.getElementById(containerId);
        if (!container) {
            console.error(`Контейнер с id ${containerId} не найден`);
            return;
        }

        container.innerHTML = this.getHeaderHTML();

        // Добавляем обработчик события для кнопки входа
        const loginBtn = container.querySelector('.nav-item.login');
        if (loginBtn) {
            loginBtn.addEventListener('click', (e) => {
                e.preventDefault();
                // Проверяем статус аутентификации
                switch (this.authStatus) {
                    case 0: // Неавторизован
                        this.showAuthModal();
                        break;
                    case 1: // Авторизован
                        window.location.href = '/user.html';
                        break;
                    default: // null или другие значения
                        console.log('Статус авторизации не определён');
                        break;
                }
            });
        }

        // Инициализируем данные пользователя
        this.initialize();
    }

    getHeaderHTML() {
        return `
            <header class="header">
                <div class="header-container">
                    <div class="logo">
                        <a href="/">
                            <img src="img/logo.png" alt="Fzon" onerror="this.classList.add('error')">
                            <span class="logo-text">Fzon</span>
                        </a>
                    </div>

                    <div class="catalog-btn">
                        <div class="squares">
                            <span></span>
                            <span></span>
                            <span></span>
                            <span></span>
                        </div>
                        <span>Каталог</span>
                    </div>

                    <div class="search">
                        <input type="text" placeholder="Искать на Fzon">
                        <button class="search-btn">
                            <svg viewBox="0 0 24 24">
                                <path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14z"/>
                            </svg>
                        </button>
                    </div>

                    <nav class="nav">
                        <a href="#" class="nav-item login">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"></path>
                                <circle cx="12" cy="7" r="4"></circle>
                            </svg>
                            <span>Войти</span>
                        </a>
                        <a href="orders.html" class="nav-item">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M6 2L3 6v14a2 2 0 0 0 2 2h14a2 2 0 0 0 2-2V6l-3-4z"></path>
                                <line x1="3" y1="6" x2="21" y2="6"></line>
                                <path d="M16 10a4 4 0 0 1-8 0"></path>
                            </svg>
                            <span>Заказы</span>
                        </a>
                        <a href="bank.html" class="nav-item">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M21 5H3a2 2 0 0 0-2 2v10a2 2 0 0 0 2 2h18a2 2 0 0 0 2-2V7a2 2 0 0 0-2-2z"></path>
                                <path d="M3 10h18"></path>
                                <path d="M16 14h4"></path>
                            </svg>
                            <span>Банк</span>
                        </a>
                        <a href="cart.html" class="nav-item">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <circle cx="9" cy="21" r="1"></circle>
                                <circle cx="20" cy="21" r="1"></circle>
                                <path d="M1 1h4l2.68 13.39a2 2 0 0 0 2 1.61h9.72a2 2 0 0 0 2-1.61L23 6H6"></path>
                            </svg>
                            <span>Корзина</span>
                            <span class="cart-count">0</span>
                        </a>
                    </nav>
                </div>
            </header>
        `;
    }
}

