class CatalogButtonsControl {
    constructor(header, catalog) {
        this.header = header;
        this.catalog = catalog;
        this.productsGrid = document.querySelector('.products-grid');
        this.token = localStorage.getItem('jwt_token');
        this.init();
    }

    init() {
        this.setupEventListeners();
    }

    setupEventListeners() {
        // Делегирование событий на контейнере товаров
        this.productsGrid.addEventListener('click', (event) => {
            this.handleProductClick(event);
        });
    }

    handleProductClick(event) {
        const target = event.target;
        const card = target.closest('.product-card');
        
        if (!card) return;

        const article = card.dataset.article;

        // Определяем тип действия
        if (target.closest('.add-to-cart')) {
            this.addToCart(article);
        } else if (target.closest('.quantity-control-btn')) {
            const btn = target.closest('.quantity-control-btn');
            const isIncrease = btn.querySelector('path[d*="M8 1.333"]'); // Упрощенная проверка для увеличения
            if (isIncrease) {
                this.changeQuantity(article, 1);
            } else {
                this.changeQuantity(article, -1);
            }
        }
    }

    async addToCart(article) {
        await this.changeQuantity(article, 1);
    }

    async changeQuantity(article, delta) {
        try {
            const response = await fetch('/api/cartservice/change-cart-product-count', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Authorization': `Bearer ${this.token}`
                },
                body: JSON.stringify({
                    article: article,
                    delta: delta
                })
            });

            if (response.ok) {
                const data = await response.json();
                
                // Обновляем хедер
                this.header.setCartCount(data.totalCount);
                
                // Обновляем каталог
                this.catalog.updateProductQuantity(article, data.productCount);
                
            } else if (response.status === 401 || response.status === 403) {
                // Токен недействителен
                localStorage.removeItem('jwt_token');
                this.token = null;
                console.error('Токен недействителен');
            } else {
                console.error('Ошибка сервера:', response.status);
            }
        } catch (error) {
            console.error('Ошибка при изменении количества товара:', error);
        }
    }
}

