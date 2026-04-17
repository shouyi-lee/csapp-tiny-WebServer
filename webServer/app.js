(function () {
    var root = document.documentElement;
    var themeToggle = document.getElementById("themeToggle");
    var themeLabel = document.getElementById("themeLabel");
    var clock = document.getElementById("clock");
    var year = document.getElementById("year");
    var serverHost = document.getElementById("serverHost");
    var statusButton = document.getElementById("statusButton");
    var statusPill = document.getElementById("statusPill");
    var statusText = document.getElementById("statusText");
    var noticeText = document.getElementById("noticeText");
    var highlightButton = document.getElementById("highlightButton");
    var resetButton = document.getElementById("resetButton");

    function readStoredTheme() {
        try {
            return window.localStorage.getItem("tiny-webserver-theme");
        } catch (error) {
            return null;
        }
    }

    function storeTheme(theme) {
        try {
            window.localStorage.setItem("tiny-webserver-theme", theme);
        } catch (error) {
            /* ignore storage failures */
        }
    }

    function getPreferredTheme() {
        var savedTheme = readStoredTheme();
        if (savedTheme === "dark" || savedTheme === "light") {
            return savedTheme;
        }

        if (window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches) {
            return "dark";
        }

        return "light";
    }

    function applyTheme(theme) {
        root.setAttribute("data-theme", theme);

        if (themeLabel) {
            themeLabel.textContent = theme.toUpperCase();
        }

        if (themeToggle) {
            themeToggle.textContent = theme === "dark" ? "浅色模式" : "深色模式";
        }
    }

    function formatTime(date) {
        return date.toLocaleTimeString("zh-CN", {
            hour12: false
        });
    }

    function updateClock() {
        if (!clock) {
            return;
        }

        clock.textContent = formatTime(new Date());
    }

    function setNotice(message) {
        if (noticeText) {
            noticeText.textContent = message;
        }
    }

    function refreshStatus() {
        var host = window.location.origin || window.location.href;
        var path = window.location.pathname || "/";

        if (serverHost) {
            serverHost.textContent = host + path;
        }

        if (statusPill) {
            statusPill.textContent = "ONLINE";
        }

        if (statusText) {
            statusText.textContent = "静态资源已成功加载：index.html、styles.css、app.js。";
        }

        setNotice("状态已刷新，当前页面路径为 " + path + " 。");
    }

    function removeHighlight() {
        document.body.classList.remove("highlight-cards");
    }

    if (year) {
        year.textContent = String(new Date().getFullYear());
    }

    applyTheme(getPreferredTheme());
    updateClock();
    refreshStatus();
    setNotice("页面已加载，可以开始浏览与测试。 ");

    window.setInterval(updateClock, 1000);

    if (themeToggle) {
        themeToggle.addEventListener("click", function () {
            var nextTheme = root.getAttribute("data-theme") === "dark" ? "light" : "dark";
            applyTheme(nextTheme);
            storeTheme(nextTheme);
            setNotice("主题已切换为 " + nextTheme.toUpperCase() + " 模式。");
        });
    }

    if (statusButton) {
        statusButton.addEventListener("click", function () {
            refreshStatus();
        });
    }

    if (highlightButton) {
        highlightButton.addEventListener("click", function () {
            document.body.classList.add("highlight-cards");
            setNotice("特性卡片已高亮，说明脚本交互正常。 ");
        });
    }

    if (resetButton) {
        resetButton.addEventListener("click", function () {
            removeHighlight();
            refreshStatus();
            setNotice("页面状态已重置。 ");
        });
    }
})();