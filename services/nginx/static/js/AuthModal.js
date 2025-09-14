class AuthModal {
    constructor(header) {
        this.header = header;
        this.isLoginMode = true;
        this.modalElement = null;
        this.init();
    }

    init() {
        this.createModal();
        this.bindEvents();
    }

    createModal() {
        const modalHTML = `
            <div class="auth-modal-overlay">
                <div class="auth-modal">
                    <div class="auth-modal-header">
                        <h2>Войти</h2>
                        <button class="close-modal">&times;</button>
                    </div>
                    <div class="auth-modal-content">
                        <form id="auth-form">
                            <div class="form-group">
                                <input type="text" id="username" name="username" required placeholder="Логин">
                            </div>
                            <div class="form-group">
                                <input type="password" id="password" name="password" required placeholder="Пароль">
                            </div>
                            <button type="submit" class="auth-submit-btn">Войти</button>
                        </form>
                        <div class="auth-switch">
                            <span id="auth-switch-text">Нет аккаунта? Зарегистрироваться</span>
                        </div>
                    </div>
                </div>
            </div>
        `;

        document.body.insertAdjacentHTML('beforeend', modalHTML);
        this.modalElement = document.querySelector('.auth-modal-overlay');
    }

    bindEvents() {
        // Закрытие модального окна
        this.modalElement.querySelector('.close-modal').addEventListener('click', () => {
            this.hide();
        });

        // Клик вне модального окна
        this.modalElement.addEventListener('click', (e) => {
            if (e.target === this.modalElement) {
                this.hide();
            }
        });

        // Переключение между входом и регистрацией
        this.modalElement.querySelector('#auth-switch-text').addEventListener('click', () => {
            this.toggleMode();
        });

        // Отправка формы
        this.modalElement.querySelector('#auth-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.handleSubmit();
        });
    }

    toggleMode() {
        this.isLoginMode = !this.isLoginMode;
        
        const header = this.modalElement.querySelector('h2');
        const submitBtn = this.modalElement.querySelector('.auth-submit-btn');
        const switchText = this.modalElement.querySelector('#auth-switch-text');
        
        if (this.isLoginMode) {
            header.textContent = 'Войти';
            submitBtn.textContent = 'Войти';
            switchText.textContent = 'Нет аккаунта? Зарегистрироваться';
        } else {
            header.textContent = 'Регистрация';
            submitBtn.textContent = 'Зарегистрироваться';
            switchText.textContent = 'Уже есть аккаунт? Войти';
        }
    }

    async handleSubmit() {
        const username = this.modalElement.querySelector('#username').value;
        const password = this.modalElement.querySelector('#password').value;

        this.clearErrors();

        if (!username) {
            this.showError('username', 'Заполните это поле');
            return;
        }

        if (!password) {
            this.showError('password', 'Заполните это поле');
            return;
        }

        try {
            const endpoint = this.isLoginMode ? '/api/auth/login' : '/api/auth/registration';
            const response = await fetch(endpoint, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ username, password })
            });

            if (response.ok) {
                const data = await response.json();
                localStorage.setItem('jwt_token', data.token);
                window.location.reload();
            } else {
                const error = await response.json();
                if (error.field) {
                    this.showError(error.field, error.message);
                } else {
                    this.showError('general', error.message || 'Произошла ошибка');
                }
            }
        } catch (error) {
            this.showError('general', 'Произошла ошибка при соединении с сервером');
        }
    }

    show() {
        this.modalElement.style.display = 'flex';
        document.body.style.overflow = 'hidden'; // Запрещаем прокрутку страницы
    }

    hide() {
        this.modalElement.style.display = 'none';
        document.body.style.overflow = ''; // Разрешаем прокрутку страницы
    }
}

