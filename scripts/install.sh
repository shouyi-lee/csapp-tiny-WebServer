#!/usr/bin/env bash

set -Eeuo pipefail

PROGRAM_NAME="tiny-server"
SERVICE_NAME="${PROGRAM_NAME}.service"

DEFAULT_REPOSITORY="https://github.com/shouyi-lee/csapp-tiny-WebServer"
DEFAULT_SOURCE_REF="master"

RUNTIME_BIN="/usr/local/bin/${PROGRAM_NAME}"
RUNTIME_SHARE_DIR="/usr/local/share/${PROGRAM_NAME}"
RUNTIME_CONFIG_DIR="/etc/${PROGRAM_NAME}"
RUNTIME_CONFIG_FILE="${RUNTIME_CONFIG_DIR}/server.conf"
RUNTIME_WEB_ROOT="/srv/${PROGRAM_NAME}/www"
RUNTIME_LOG_DIR="/var/log/${PROGRAM_NAME}"
RUNTIME_LOG_FILE="${RUNTIME_LOG_DIR}/server.log"
RUNTIME_UNIT_FILE="/etc/systemd/system/${SERVICE_NAME}"

REPOSITORY="${TINY_SERVER_REPOSITORY:-$DEFAULT_REPOSITORY}"
SOURCE_REF="${TINY_SERVER_REF:-$DEFAULT_SOURCE_REF}"
SOURCE_SHA256="${TINY_SERVER_SHA256:-}"
LOCAL_SOURCE_DIR=""
DESTDIR="${DESTDIR:-}"
PORT="1145"
REAL_IP_HEADER="CF-Connecting-IP"
START_SERVICE=1
FORCE_WEB=0

WORK_DIR=""
SOURCE_TREE=""
CONFIG_CREATED=0
WEB_INSTALLED=0

usage()
{
    cat <<'EOF'
Usage: install.sh [options]

Download, compile and install tiny-server as a systemd system service.
Run this script as a normal user; it asks sudo only for installation steps.

Options:
  --ref REF              Git branch, tag or commit to download (default: master)
  --repository URL       Source repository URL
  --sha256 HASH          Require this SHA-256 for the downloaded source archive
  --source-dir DIR       Build a local source tree instead of downloading
  --port PORT            Port written to a newly created config (default: 1145)
  --real-ip-header NAME  Header written to a newly created config
  --force-web            Copy example web files even when the web root is nonempty
  --no-start             Install files but do not enable or start the service
  --destdir DIR          Stage files below DIR; implies --no-start and never uses sudo
  -h, --help             Show this help

Environment equivalents:
  TINY_SERVER_REPOSITORY, TINY_SERVER_REF, TINY_SERVER_SHA256, DESTDIR

Existing /etc/tiny-server/server.conf and web-root contents are preserved by
default. The binary, example config and systemd unit are updated on each run.
EOF
}

info()
{
    printf 'info: %s\n' "$*"
}

warn()
{
    printf 'warning: %s\n' "$*" >&2
}

die()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

cleanup()
{
    if [[ -n "$WORK_DIR" && -d "$WORK_DIR" ]]; then
        rm -rf -- "$WORK_DIR"
    fi
}

trap cleanup EXIT

need_command()
{
    command -v "$1" >/dev/null 2>&1 ||
        die "required command not found: $1"
}

as_root()
{
    if [[ -n "$DESTDIR" || "$EUID" -eq 0 ]]; then
        "$@"
    else
        sudo "$@"
    fi
}

install_path()
{
    printf '%s%s' "$DESTDIR" "$1"
}

directory_has_entries()
{
    local directory="$1"
    local entries

    [[ -d "$directory" ]] || return 1
    shopt -s nullglob dotglob
    entries=("$directory"/*)
    shopt -u nullglob dotglob
    ((${#entries[@]} > 0))
}

validate_options()
{
    [[ "$PORT" =~ ^[0-9]+$ ]] ||
        die "port must contain decimal digits only: $PORT"
    ((10#$PORT >= 1 && 10#$PORT <= 65535)) ||
        die "port must be between 1 and 65535: $PORT"

    [[ "$REAL_IP_HEADER" =~ ^[A-Za-z0-9-]+$ ]] ||
        die "real IP header may contain letters, digits and '-' only"

    [[ -n "$LOCAL_SOURCE_DIR" || "$SOURCE_REF" =~ ^[A-Za-z0-9._/-]+$ ]] ||
        die "invalid source ref: $SOURCE_REF"
    [[ "$SOURCE_REF" != -* ]] ||
        die "source ref must not begin with '-'"

    if [[ -n "$SOURCE_SHA256" ]]; then
        [[ "$SOURCE_SHA256" =~ ^[A-Fa-f0-9]{64}$ ]] ||
            die "--sha256 must be exactly 64 hexadecimal characters"
    fi

    if [[ -n "$DESTDIR" ]]; then
        [[ "$DESTDIR" == /* ]] ||
            die "--destdir must be an absolute path"
        [[ "$DESTDIR" != "/" ]] ||
            die "use an empty DESTDIR for a real system installation"
        DESTDIR="${DESTDIR%/}"
        START_SERVICE=0
    fi
}

parse_options()
{
    while (($# > 0)); do
        case "$1" in
            --ref)
                (($# >= 2)) || die "--ref requires a value"
                SOURCE_REF="$2"
                shift 2
                ;;
            --repository)
                (($# >= 2)) || die "--repository requires a value"
                REPOSITORY="$2"
                shift 2
                ;;
            --sha256)
                (($# >= 2)) || die "--sha256 requires a value"
                SOURCE_SHA256="$2"
                shift 2
                ;;
            --source-dir)
                (($# >= 2)) || die "--source-dir requires a value"
                LOCAL_SOURCE_DIR="$2"
                shift 2
                ;;
            --port)
                (($# >= 2)) || die "--port requires a value"
                PORT="$2"
                shift 2
                ;;
            --real-ip-header)
                (($# >= 2)) || die "--real-ip-header requires a value"
                REAL_IP_HEADER="$2"
                shift 2
                ;;
            --force-web)
                FORCE_WEB=1
                shift
                ;;
            --no-start)
                START_SERVICE=0
                shift
                ;;
            --destdir)
                (($# >= 2)) || die "--destdir requires a value"
                DESTDIR="$2"
                shift 2
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                die "unknown option: $1"
                ;;
        esac
    done
}

prepare_source()
{
    SOURCE_TREE="${WORK_DIR}/source"
    mkdir -p "$SOURCE_TREE"

    if [[ -n "$LOCAL_SOURCE_DIR" ]]; then
        [[ -d "$LOCAL_SOURCE_DIR" ]] ||
            die "local source directory does not exist: $LOCAL_SOURCE_DIR"
        info "copying local source tree: $LOCAL_SOURCE_DIR"
        cp -a "${LOCAL_SOURCE_DIR}/." "$SOURCE_TREE/"
        return
    fi

    local repository_base="${REPOSITORY%.git}"
    local archive_url="${repository_base}/archive/${SOURCE_REF}.tar.gz"
    local archive="${WORK_DIR}/source.tar.gz"

    info "downloading source: $archive_url"
    curl --fail --location --silent --show-error \
        --retry 3 --proto '=https' --tlsv1.2 \
        --output "$archive" "$archive_url"

    if [[ -n "$SOURCE_SHA256" ]]; then
        local actual_sha256
        actual_sha256="$(sha256sum "$archive" | awk '{print $1}')"
        [[ "${actual_sha256,,}" == "${SOURCE_SHA256,,}" ]] ||
            die "source SHA-256 mismatch (actual: $actual_sha256)"
        info "source SHA-256 verified"
    else
        warn "source archive was not checksum-pinned; use --sha256 for a reproducible release"
    fi

    tar -xzf "$archive" --strip-components=1 -C "$SOURCE_TREE"
}

build_source()
{
    [[ -f "${SOURCE_TREE}/Makefile" ]] ||
        die "downloaded source does not contain Makefile"
    [[ -d "${SOURCE_TREE}/website" ]] ||
        die "downloaded source does not contain website/"

    local jobs="1"
    if command -v getconf >/dev/null 2>&1; then
        jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')"
    fi
    [[ "$jobs" =~ ^[1-9][0-9]*$ ]] || jobs="1"

    info "compiling tiny-server with $jobs job(s)"
    make -C "$SOURCE_TREE" clean
    make -C "$SOURCE_TREE" -j "$jobs"

    [[ -x "${SOURCE_TREE}/webserver" ]] ||
        die "build completed without producing an executable webserver"
}

generate_install_files()
{
    cat >"${WORK_DIR}/server.conf.example" <<EOF
port: ${PORT}
log_url: ${RUNTIME_LOG_FILE}
source_url: ${RUNTIME_WEB_ROOT}
root_source: index.html
real_ip_head: ${REAL_IP_HEADER}
EOF

    cat >"${WORK_DIR}/${SERVICE_NAME}" <<EOF
[Unit]
Description=Tiny HTTP Server
Documentation=https://github.com/shouyi-lee/csapp-tiny-WebServer
After=network.target
StartLimitIntervalSec=30s
StartLimitBurst=5

[Service]
Type=exec
DynamicUser=yes
ExecStart=${RUNTIME_BIN} ${RUNTIME_CONFIG_FILE}

LogsDirectory=${PROGRAM_NAME}
LogsDirectoryMode=0750
UMask=0027

Restart=on-failure
RestartSec=2s
RestartPreventExitStatus=255

StandardOutput=journal
StandardError=journal
SyslogIdentifier=${PROGRAM_NAME}

NoNewPrivileges=yes
PrivateTmp=yes
PrivateDevices=yes
ProtectSystem=strict
ProtectHome=yes
ProtectKernelTunables=yes
ProtectKernelModules=yes
ProtectControlGroups=yes
RestrictAddressFamilies=AF_UNIX AF_NETLINK AF_INET AF_INET6

[Install]
WantedBy=multi-user.target
EOF
}

install_files()
{
    local bin_path
    local share_dir
    local config_dir
    local config_file
    local web_root
    local unit_file

    bin_path="$(install_path "$RUNTIME_BIN")"
    share_dir="$(install_path "$RUNTIME_SHARE_DIR")"
    config_dir="$(install_path "$RUNTIME_CONFIG_DIR")"
    config_file="$(install_path "$RUNTIME_CONFIG_FILE")"
    web_root="$(install_path "$RUNTIME_WEB_ROOT")"
    unit_file="$(install_path "$RUNTIME_UNIT_FILE")"

    info "installing executable: $bin_path"
    as_root install -Dm755 "${SOURCE_TREE}/webserver" "$bin_path"

    as_root install -d -m0755 "$share_dir"
    as_root install -m0644 \
        "${WORK_DIR}/server.conf.example" \
        "${share_dir}/server.conf.example"

    as_root install -d -m0755 "$config_dir"
    if [[ -e "$config_file" ]]; then
        warn "preserving existing config: $config_file"
    else
        info "installing initial config: $config_file"
        as_root install -m0644 "${WORK_DIR}/server.conf.example" "$config_file"
        CONFIG_CREATED=1
    fi

    as_root install -d -m0755 "$web_root"
    if directory_has_entries "$web_root" && ((FORCE_WEB == 0)); then
        warn "preserving nonempty web root: $web_root"
    else
        info "installing example website: $web_root"
        as_root cp -a "${SOURCE_TREE}/website/." "${web_root}/"
        as_root chmod -R u=rwX,go=rX "$web_root"
        if [[ -z "$DESTDIR" ]]; then
            as_root chown -R root:root "$web_root"
        fi
        WEB_INSTALLED=1
    fi

    info "installing systemd unit: $unit_file"
    as_root install -Dm644 "${WORK_DIR}/${SERVICE_NAME}" "$unit_file"
}

activate_service()
{
    if [[ -n "$DESTDIR" ]]; then
        info "staged installation complete; systemd was not contacted"
        return
    fi

    if [[ ! -d /run/systemd/system ]]; then
        warn "systemd is not running; unit installed but service not activated"
        return
    fi

    info "reloading systemd units"
    as_root systemctl daemon-reload

    if ((START_SERVICE == 0)); then
        info "service was not enabled or started (--no-start)"
        return
    fi

    info "enabling and starting ${SERVICE_NAME}"
    as_root systemctl enable "$SERVICE_NAME"
    if as_root systemctl is-active --quiet "$SERVICE_NAME"; then
        as_root systemctl restart "$SERVICE_NAME"
    else
        as_root systemctl start "$SERVICE_NAME"
    fi
}

show_summary()
{
    printf '\nInstallation summary:\n'
    printf '  executable: %s\n' "$(install_path "$RUNTIME_BIN")"
    printf '  config:     %s%s\n' \
        "$(install_path "$RUNTIME_CONFIG_FILE")" \
        "$([[ "$CONFIG_CREATED" -eq 1 ]] && printf ' (created)' || printf ' (preserved)')"
    printf '  web root:   %s%s\n' \
        "$(install_path "$RUNTIME_WEB_ROOT")" \
        "$([[ "$WEB_INSTALLED" -eq 1 ]] && printf ' (installed)' || printf ' (preserved)')"
    printf '  log file:   %s\n' "$(install_path "$RUNTIME_LOG_FILE")"
    printf '  unit:       %s\n' "$(install_path "$RUNTIME_UNIT_FILE")"

    if [[ -z "$DESTDIR" && -d /run/systemd/system ]]; then
        printf '\nUseful commands:\n'
        printf '  sudo systemctl status %s\n' "$PROGRAM_NAME"
        printf '  sudo journalctl -u %s -n 50 --no-pager\n' "$PROGRAM_NAME"
        printf '  sudoedit %s\n' "$RUNTIME_CONFIG_FILE"
    fi
}

main()
{
    parse_options "$@"
    validate_options

    need_command make
    need_command gcc
    need_command install
    need_command cp
    need_command tar

    if [[ -z "$LOCAL_SOURCE_DIR" ]]; then
        need_command curl
        [[ -z "$SOURCE_SHA256" ]] || need_command sha256sum
    fi
    if [[ -z "$DESTDIR" && "$EUID" -ne 0 ]]; then
        need_command sudo
    fi
    if [[ -z "$DESTDIR" && -d /run/systemd/system ]]; then
        need_command systemctl
    fi

    WORK_DIR="$(mktemp -d "${TMPDIR:-/tmp}/${PROGRAM_NAME}-install.XXXXXX")"

    prepare_source
    build_source
    generate_install_files
    install_files
    activate_service
    show_summary
}

main "$@"
