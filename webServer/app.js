(function () {
    var root = document.documentElement;
    var themeToggle = document.getElementById("themeToggle");
    var themeToggleText = document.getElementById("themeToggleText");
    var themeLabel = document.getElementById("themeLabel");
    var clock = document.getElementById("clock");
    var year = document.getElementById("year");
    var serverOrigin = document.getElementById("serverOrigin");
    var visitorPath = document.getElementById("visitorPath");
    var statusButton = document.getElementById("statusButton");
    var pulseButton = document.getElementById("pulseButton");
    var statusPill = document.getElementById("statusPill");
    var statusText = document.getElementById("statusText");
    var noticeText = document.getElementById("noticeText");
    var navLinks = Array.prototype.slice.call(document.querySelectorAll("[data-nav-target]"));
    var sectionIds = navLinks.map(function (link) {
        return link.getAttribute("data-nav-target");
    });
    var sections = sectionIds.map(function (id) {
        return document.getElementById(id);
    }).filter(Boolean);
    var pulseTimer = null;
    var messageIndex = 0;
    var messages = [
        "星际脉冲已发出，愿下一次合作在星云之间相遇。",
        "导航阵列稳定，这里已准备好替换成你的真实经历与作品。",
        "静态引擎运行正常，tiny-server 足以承载这座浪漫的个人星站。",
        "星图校准完成，继续向下浏览即可查看技能、任务档案与联络信标。"
    ];

    function readStoredTheme() {
        try {
            return window.localStorage.getItem("star-harbor-theme");
        } catch (error) {
            return null;
        }
    }

    function storeTheme(theme) {
        try {
            window.localStorage.setItem("star-harbor-theme", theme);
        } catch (error) {
            /* ignore storage failures */
        }
    }

    function getPreferredTheme() {
        var savedTheme = readStoredTheme();
        if (savedTheme === "dark" || savedTheme === "light") {
            return savedTheme;
        }

        if (window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches) {
            return "light";
        }

        return "dark";
    }

    function setNotice(message) {
        if (noticeText) {
            noticeText.textContent = message;
        }
    }

    function applyTheme(theme) {
        root.setAttribute("data-theme", theme);

        if (themeLabel) {
            themeLabel.textContent = theme === "dark" ? "DEEP SPACE" : "AURORA";
        }

        if (themeToggleText) {
            themeToggleText.textContent = theme === "dark" ? "切换为晨曦模式" : "切换为深空模式";
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

    function refreshStatus() {
        var origin = window.location.origin || (window.location.protocol + "//" + window.location.host) || "本地预览";
        var path = window.location.pathname || "/";

        if (serverOrigin) {
            serverOrigin.textContent = origin;
        }

        if (visitorPath) {
            visitorPath.textContent = path;
        }

        if (statusPill) {
            statusPill.textContent = "ONLINE";
        }

        if (statusText) {
            statusText.textContent = "静态页面、样式、脚本与外部图像资源已完成对接，可直接由 tiny-server 提供。";
        }

        setNotice("已校准访问坐标：" + origin + path + " 。");
    }

    function activateNav(targetId) {
        navLinks.forEach(function (link) {
            link.classList.toggle("is-active", link.getAttribute("data-nav-target") === targetId);
        });
    }

    function triggerPulse() {
        document.body.classList.remove("signal-active");
        void document.body.offsetWidth;
        document.body.classList.add("signal-active");

        if (statusPill) {
            statusPill.textContent = "PULSE";
        }

        setNotice(messages[messageIndex % messages.length]);
        messageIndex += 1;

        if (pulseTimer) {
            window.clearTimeout(pulseTimer);
        }

        pulseTimer = window.setTimeout(function () {
            document.body.classList.remove("signal-active");
            if (statusPill) {
                statusPill.textContent = "ONLINE";
            }
        }, 1500);
    }

    function setupObserver() {
        if (!("IntersectionObserver" in window) || sections.length === 0) {
            return;
        }

        var observer = new window.IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) {
                    activateNav(entry.target.id);
                }
            });
        }, {
            rootMargin: "-35% 0px -45% 0px",
            threshold: 0.12
        });

        sections.forEach(function (section) {
            observer.observe(section);
        });
    }

    if (year) {
        year.textContent = String(new Date().getFullYear());
    }

    applyTheme(getPreferredTheme());
    updateClock();
    refreshStatus();
    setNotice("欢迎来到个人星港，向下滚动即可浏览关于、技能、任务与联络信标。 ");
    setupObserver();

    if (window.location.hash) {
        activateNav(window.location.hash.replace("#", ""));
    } else if (sections.length > 0) {
        activateNav(sections[0].id);
    }

    window.setInterval(updateClock, 1000);

    if (themeToggle) {
        themeToggle.addEventListener("click", function () {
            var nextTheme = root.getAttribute("data-theme") === "light" ? "dark" : "light";
            applyTheme(nextTheme);
            storeTheme(nextTheme);
            setNotice(nextTheme === "dark" ? "已切换为深空模式。" : "已切换为晨曦模式。 ");
        });
    }

    if (statusButton) {
        statusButton.addEventListener("click", refreshStatus);
    }

    if (pulseButton) {
        pulseButton.addEventListener("click", triggerPulse);
    }

    window.addEventListener("hashchange", function () {
        var currentId = window.location.hash.replace("#", "");
        if (currentId) {
            activateNav(currentId);
        }
    });
})();
