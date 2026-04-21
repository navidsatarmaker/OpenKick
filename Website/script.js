// Smooth scroll for anchor links
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
        e.preventDefault();
        
        const targetId = this.getAttribute('href');
        const targetElement = document.querySelector(targetId);
        
        if (targetElement) {
            targetElement.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });
        }
    });
});

// Dynamic Oscilloscope rendering for Hero visual effect
const scopeGrid = document.querySelector('.scope-grid');
if (scopeGrid) {
    // Generate some fake UI lines to represent the plugin scope
    for (let i = 0; i < 20; i++) {
        const line = document.createElement('div');
        line.style.position = 'absolute';
        line.style.left = `${i * 5}%`;
        line.style.height = '100%';
        line.style.width = '1px';
        line.style.background = 'rgba(255,255,255,0.05)';
        scopeGrid.appendChild(line);
    }
}
