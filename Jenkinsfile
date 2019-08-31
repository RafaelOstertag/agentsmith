pipeline {
    agent none

    options {
        ansiColor('xterm')
        buildDiscarder logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '', daysToKeepStr: '', numToKeepStr: '10')
        timeout(time: 1, unit: 'HOURS')
        timestamps()
        disableConcurrentBuilds()
    }

    environment {
        PEDANTIC_FLAGS = "-Wall -pedantic -Werror -O3 -Wno-unknown-pragmas"
    }

    triggers {
        pollSCM '@hourly'
    }

    stages {
        stage("OS Build") {
            parallel {
                stage("FreeBSD amd64") {
                    agent {
                        label "freebsd&&amd64"
                    }
                    stages {
                        stage("(FB64) Bootstrap Build") {
                             steps {
                                sh "git log --stat > ChangeLog"
                                sh "autoreconf -I m4 -i"
                            }
                        }

                        stage("(FB64) Configure") {
                            steps {
                                dir("obj") {
                                    sh '../configure CPPFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib -R/usr/local/lib"'
                                }
                            }
                        }

						stage("(FB64) Build") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE all CFLAGS="${PEDANTIC_FLAGS}"'
                                }
                            }
                        }

                        stage("(FB64) Test") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE check'
                                }
                            }
                        }

                        stage("Build distribution") {
                            when { 
                                branch 'release/*'
                            }

                            steps {
                                sh '$MAKE distcheck'
                                dir("obj") {
                                    sshagent(['897482ed-9233-4d56-88c3-254b909b6316']) {
                                        sh """sftp ec2-deploy@ec2-52-29-59-221.eu-central-1.compute.amazonaws.com:/data/www/agentsmith.guengel.ch/downloads <<EOF
                                    put ${packageName}-${version}.tar.gz
                                    put ${packageName}-${version}.tar.bz2
                                    put ${packageName}-${version}.tar.xz
                                    EOF
                                    """
                                    }
                                }
                            }

                            post {
                                always {
                                    // If distcheck fails, it leaves certain directories with read-only permissions.
                                    // We unconditionally set write mode
                                    dir("obj") {
                                        sh "chmod -R u+w ."
                                    }

                                    cleanWs notFailBuild: true
                                }
                            }
                        }
                    }
                } // stage("FreeBSD amd64")

				stage("Linux") {
					agent {
						label "linux"
					}
					stages {
						stage("(LX) Bootstrap Build") {
                             steps {
                                sh "touch ChangeLog"
                                sh "autoreconf -I m4 -i"
                            }
                        }

                        stage("(LX) Configure") {
                            steps {
                                dir("obj") {
                                    sh '../configure'
                                }
                            }
                        }

                        stage("(LX) Build") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE all CFLAGS="${PEDANTIC_FLAGS}"'
                                }
                            }
                        }

                        stage("(LX) Test") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE check'
                                }
                            }
                        }
					}
				} // stage("Linux")

				stage("OpenBSD amd64") {
					agent {
						label "openbsd&&amd64"
					}
					stages {
						stage("(OB64) Bootstrap Build") {
                             steps {
                                sh "touch ChangeLog"
                                sh "autoreconf -I m4 -i"
                            }
                        }

                        stage("(OB64) Configure") {
                            steps {
                                dir("obj") {
                                    sh '../configure CC=cc CXX=c++ CPPFLAGS="-I/usr/local/include" LDFLAGS="-L/usr/local/lib -R/usr/local/lib"'
                                }
                            }
                        }

						stage("(OB64) Build") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE all CFLAGS="${PEDANTIC_FLAGS}"'
                                }
                             }
                        }
                        
                        stage("(OB64) Test") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE check'
                                }
                            }
                        }
					}
				} // stage("OpenBSD amd64")

			    stage("NetBSD") {
                    agent {
                        label "netbsd&&amd64"
                    }
    			    stages {
						stage("(NB) Bootstrap Build") {
                             steps {
                                sh "touch ChangeLog"
                                sh "autoreconf -I m4 -i"
                            }
                        }

                        stage("(NB) Configure") {
                            steps {
                                dir("obj") {
                                    sh "../configure CPPFLAGS='-I/usr/pkg/include' LDFLAGS='-L/usr/pkg/lib -R/usr/pkg/lib'"
                                }
                            }
                        }

                        stage("(NB) Build") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE all CFLAGS="${PEDANTIC_FLAGS}"'
                                }
                            }
                        }

                        stage("(NB) Test") {
                            steps {
                                dir("obj") {
                                    sh '$MAKE check'
                                }
                            }
                        }
					}
				} // stage("NetBSD")
    		} // parallel
        } // stage("OS Build")
    } // stages

    post {
        always {
            mail to: "rafi@guengel.ch",
                    subject: "${JOB_NAME} (${BRANCH_NAME};${env.BUILD_DISPLAY_NAME}) -- ${currentBuild.currentResult}",
                    body: "Refer to ${currentBuild.absoluteUrl}"
        }
    }
}
