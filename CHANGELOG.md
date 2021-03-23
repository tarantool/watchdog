# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2021-03-23

### Added

- New option to enable core dumps.

## [1.0.2] - 2019-12-09

### Fixed

- Fix segmentation fault when user calls `stop()` twice.

### Other

- Bring luarocks best practices from `tarantool/modulekit`.

## [1.0.1] - 2018-02-22

### Fixed

- Unnecessary abortion after laptop is waken from suspend.

## [1.0.0] - 2018-01-18

### Added

- Basic functionality
- Luarock-based packaging
- Unit tests
- Travis CI integration
