class Header {
    constructor() {
        this.cartCount = 0;
    }

    setCartCount(count) {
        this.cartCount = count;
        this.updateCartCount();
    }

    updateCartCount() {
        const cartCountElement = document.querySelector('.cart-count');
        if (cartCountElement) {
            if (this.cartCount > 0) {
                cartCountElement.textContent = this.cartCount;
                cartCountElement.style.display = 'block';
                
                if (this.cartCount > 9) {
                    cartCountElement.classList.add('double-digit');
                } else {
                    cartCountElement.classList.remove('double-digit');
                }
            } else {
                cartCountElement.style.display = 'none';
            }
        }
    }

    render(containerId) {
        const container = document.getElementById(containerId);
        if (!container) {
            console.error(`Container with id ${containerId} not found`);
            return;
        }

        container.innerHTML = this.getHeaderHTML();
        
        // Инициализируем счетчик корзины
        this.updateCartCount();
    }

    getHeaderHTML() {
        return `
            <header class="header">
                <div class="header-container">
                    <div class="logo">
                        <a href="index.html">
                            <img src="logo.png" alt="Fzon" onerror="this.style.display='none';">
                            <span class="logo-text">fzon</span>
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
                        <a href="login.html" class="nav-item">
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
                                <circle cx="12" cy="12" r="10"></circle>
                                <circle cx="12" cy="12" r="3"></circle>
                                <line x1="12" y1="2" x2="12" y2="4"></line>
                                <line x1="12" y1="20" x2="12" y2="22"></line>
                                <line x1="4.93" y1="4.93" x2="6.34" y2="6.34"></line>
                                <line x1="17.66" y1="17.66" x2="19.07" y2="19.07"></line>
                                <line x1="2" y1="12" x2="4" y2="12"></line>
                                <line x1="20" y1="12" x2="22" y2="12"></line>
                                <line x1="4.93" y1="19.07" x2="6.34" y2="17.66"></line>
                                <line x1="17.66" y1="6.34" x2="19.07" y2="4.93"></line>
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

