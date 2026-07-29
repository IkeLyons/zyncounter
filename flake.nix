{
  description = "Zyncounter - ESP32 magnet event tracker";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages.default = pkgs.buildGoModule {
          pname = "zyncounter";
          version = "0.1.0";
          src = ./.;
          vendorHash = "sha256-dsmRXd5moOA08U2Hbi9Z3Hy1inZFiDOD9AMS56uk+8g=";
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            go
            gotools
            sqlite
          ];
        };
      }
    )
    //
    {
      nixosModules.default = { config, lib, pkgs, ... }:
        let
          cfg = config.services.zyncounter;
        in {
          options.services.zyncounter = {
            enable = lib.mkEnableOption "zyncounter event tracker";

            port = lib.mkOption {
              type = lib.types.port;
              default = 8080;
            };

            dbPath = lib.mkOption {
              type = lib.types.str;
              default = "/var/lib/zyncounter/events.db";
            };

            # Path to a file containing: ZYNCOUNTER_API_KEY=your-secret-here
            # Manage this with agenix, sops-nix, or place it manually.
            secretsFile = lib.mkOption {
              type = lib.types.path;
              description = "Env file with ZYNCOUNTER_API_KEY=...";
            };
          };

          config = lib.mkIf cfg.enable {
            systemd.services.zyncounter = {
              description = "Zyncounter event tracker";
              wantedBy = [ "multi-user.target" ];
              after = [ "network.target" ];
              serviceConfig = {
                ExecStart = "${self.packages.${pkgs.system}.default}/bin/zyncounter";
                EnvironmentFile = cfg.secretsFile;
                Environment = [
                  "ZYNCOUNTER_PORT=${toString cfg.port}"
                  "ZYNCOUNTER_DB_PATH=${cfg.dbPath}"
                ];
                StateDirectory = "zyncounter";
                DynamicUser = true;
                Restart = "on-failure";
              };
            };
          };
        };
    };
}
